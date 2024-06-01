#include "examples/asio/chat/codec.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/ThreadLocalSingleton.h"
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
    codec_(std::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
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
    server_.setThreadInitCallback(std::bind(&ChatServer::threadInit, this, _1));
    server_.start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    // 不需要加锁，因为每个线程都有自己的ConnectionList
    if (conn->connected())
    {
      LocalConnections::instance().insert(conn);
    }
    else
    {
      LocalConnections::instance().erase(conn);
    }
  }

  void onStringMessage(const TcpConnectionPtr&,
                       const string& message,
                       Timestamp)
  {
    EventLoop::Functor f = std::bind(&ChatServer::distributeMessage, this, message);
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    // 转发消息给所有客户端，高效转发（多线程来转发）
    for (std::set<EventLoop*>::iterator it = loops_.begin();
        it != loops_.end();
        ++it)
    {
      // 1、让对应的IO线程来执行distributeMessage
      // 2、distributeMessage放到IO线程队列中之心，因此，这里的mutex锁竞争大大减小
      // 3、distributeMessage不受mutex保护
      (*it)->queueInLoop(f);
    }
    LOG_DEBUG;
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;

  void distributeMessage(const string& message)
  {
    LOG_DEBUG << "begin";
    for (ConnectionList::iterator it = LocalConnections::instance().begin();
        it != LocalConnections::instance().end();
        ++it)
    {
      codec_.send(get_pointer(*it), message);
    }
    LOG_DEBUG << "end";
  }

  // 这个函数在每个IO线程启动前被调用
  void threadInit(EventLoop* loop)
  {
    assert(LocalConnections::pointer() == NULL);
    LocalConnections::instance();
    assert(LocalConnections::pointer() != NULL);
    MutexLockGuard lock(mutex_);
    loops_.insert(loop);
  }

  TcpServer server_;
  LengthHeaderCodec codec_;
  // 定义线程局部单例变量，每个线程都有一个ConnectionList实例
  typedef ThreadLocalSingleton<ConnectionList> LocalConnections; // 这句话提前是不是更好？

  MutexLock mutex_;
  std::set<EventLoop*> loops_ GUARDED_BY(mutex_);
};

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
 消息转发机制：
   以前：T1线程转发消息给所有的客户端
   现在：假设我们创建了4个线程   T1线程转发消息给C1、T2线程转发消息给C2
                             T3线程转发消息给C3、T4线程转发消息给C4 
                             T1线程转发消息给C5、......
        这样做的好处就是降低了hello消息到达第一个客户端与最后一个客户端之间的延迟
*/