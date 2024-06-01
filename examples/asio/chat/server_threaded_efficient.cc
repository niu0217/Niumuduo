#include "examples/asio/chat/codec.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

#include <set>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer : noncopyable
{
 public:
  ChatServer(EventLoop* loop,
             const InetAddress& listenAddr)
  : server_(loop, listenAddr, "ChatServer"),
    codec_(std::bind(&ChatServer::onStringMessage, this, _1, _2, _3)),
    connections_(new ConnectionList)
  {
    server_.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start()
  {
    server_.start();
  }

 private:
  // 写端 会修改connections_
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    MutexLockGuard lock(mutex_);
    if (!connections_.unique())  // 说明引用计数大于等于2
    {
      // 拷贝一份connections_的副本给connections_
      // 这样就将connections_和之前的connections_断开了连接
      // 因此修改connections_不会影响到读端的connections_
      connections_.reset(new ConnectionList(*connections_));
    }
    assert(connections_.unique());

    // 在副本上修改，不会影响读者，所以读者在遍历connections_的时候
    // 不需要mutex保护
    if (conn->connected())
    {
      connections_->insert(conn);
    }
    else
    {
      connections_->erase(conn);
    }
  }

  /// 借用 shared_ptr 实现 copy on write
  typedef std::set<TcpConnectionPtr> ConnectionList;
  typedef std::shared_ptr<ConnectionList> ConnectionListPtr;

  // 读操作 读取 connections_
  // 读之前: 将connections_的引用计数+1
  // 读之后: 将connections_的引用计数-1
  // 保证读connections_期间，其引用计数大于1，可以阻止并发写
  void onStringMessage(const TcpConnectionPtr&,
                       const string& message,
                       Timestamp)
  {
    // connections_的引用计数加1，mutex保护的临界区大大缩短
    ConnectionListPtr connections = getConnectionList();
    // 下面这段代码虽然不受mutex保护，写者可能会更改connections_
    // 但是实际上，写者是在另一个副本上修改的，所以无须担心
    for (ConnectionList::iterator it = connections->begin();
        it != connections->end();
        ++it)
    {
      codec_.send(get_pointer(*it), message);
    }
  } // 当connections这个栈上变量销毁的时候，connections_的引用计数减1

  ConnectionListPtr getConnectionList()
  {
    // 这里加锁的目的是为了防止connections_被其他线程修改
    MutexLockGuard lock(mutex_);
    return connections_;
  }

  TcpServer server_;
  LengthHeaderCodec codec_;
  MutexLock mutex_;
  ConnectionListPtr connections_ GUARDED_BY(mutex_);
};

// 这个程序的不足点：
//   hello消息到达第一个客户端与最后一个客户端之间的延迟比较大
int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);
    ChatServer server(&loop, serverAddr);
    if (argc > 2)
    {
      server.setThreadNum(atoi(argv[2]));
    }
    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port [thread_num]\n", argv[0]);
  }
}

/*
 降低锁竞争的方法：借助shared_ptr实现copy on write

 shared_ptr是引用计数智能指针，如果当前只有一个观察者，那么引用计数为1，可以用
 shared_ptr::unique()来判断

 对于write端，如果发现引用计数为1，这时可以安全地修改对象，不必担心有人在读它

 对于read端，在之前把引用计数加1，读完之后减1，这样可以保证在读的期间其引用计数大于1，
 可以阻止并发写

 比较难的是，对于write端，如果发现引用计数大于1，该如何处理？ 
 既然要更新数据，必定要加锁。
 如果这个时候其他线程正在读，那么不能在原来的数据上修改，得创建一个副本，在副本上修改，修改
 完了再替换。如果没有用户在读，那么可以直接修改。
*/