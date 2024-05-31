#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

const char* g_file = NULL;

// FIXME: use FileUtil::readFile()
string readFile(const char* filename)
{
  string content;
  FILE* fp = ::fopen(filename, "rb");
  if (fp)
  {
    // inefficient!!!
    const int kBufSize = 1024*1024;
    char iobuf[kBufSize];
    ::setbuffer(fp, iobuf, sizeof iobuf);

    char buf[kBufSize];
    size_t nread = 0;
    while ( (nread = ::fread(buf, 1, sizeof buf, fp)) > 0)
    {
      content.append(buf, nread);
    }
    ::fclose(fp);
  }
  return content;
}

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
  LOG_INFO << "HighWaterMark " << len;
}

/*
 关键知识点：
   1. fileContent比较大的时候，是没有办法一次性将数据拷贝到内核缓冲区的，这时候
      会将剩余的数据拷贝到应用层的OutputBuffer中。当内核缓冲区中的数据发送出去
      之后，可写事件产生，muduo就会从OutputBuffer中取出数据继续填充到内核缓存区。

   2. send函数是非阻塞的，立刻返回。不用担心数据什么时候传送给对等端。
      这个是由网络库muduo负责到底
*/
void onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << "FileServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
  if (conn->connected())
  {
    LOG_INFO << "FileServer - Sending file " << g_file
             << " to " << conn->peerAddress().toIpPort();
    // 超过64kb就调用onHighWaterMark函数
    conn->setHighWaterMarkCallback(onHighWaterMark, 64*1024);
    string fileContent = readFile(g_file); // fileContent可以会非常大，有可能为几GB
    // 一次性把文件读入到内存中，一次性调用send发送完毕
    // 这样内存消耗非常大
    conn->send(fileContent);
    /// 这里send之后，就直接shutdown了，不会有问题吗？
    /// 解释：不会。shutdown内部只是关闭了写入这一半，并且如果OutputBuffer不为空
    ///      也就是还有数据没有发送完毕，这时是不会关闭的，它会等到OutputBuffer变为空
    ///      的时候才会真正的关闭写入这一半
    conn->shutdown();
    LOG_INFO << "FileServer - done";
  }
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    g_file = argv[1];  // 保存文件名字

    EventLoop loop;
    InetAddress listenAddr(2021);
    TcpServer server(&loop, listenAddr, "FileServer");
    server.setConnectionCallback(onConnection);
    server.start();
    loop.loop();
  }
  else
  {
    fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
  }
}

