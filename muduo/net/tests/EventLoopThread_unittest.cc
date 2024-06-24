#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Thread.h"
#include "muduo/base/CountDownLatch.h"

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

void print(EventLoop* p = NULL)
{
  printf("print: pid = %d, tid = %d, loop = %p\n",
         getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop* p)
{
  print(p);
  p->quit();
}

void test1()
{
  EventLoopThread thr1;
  EventLoop *loop1 = thr1.startLoop();
  printf("loop1 = %p\n", loop1);
  loop1->runInLoop(std::bind(print, loop1));

  EventLoopThread thr2;
  EventLoop *loop2 = thr2.startLoop();
  printf("loop2 = %p\n", loop2);
  loop2->runInLoop(std::bind(print, loop2));
  CurrentThread::sleepUsec(500 * 1000);
}

int main()
{
  print();
  test1();

  // {
  // EventLoopThread thr1;  // never start
  // }

  // {
  // // dtor calls quit()
  // EventLoopThread thr2;
  // EventLoop* loop = thr2.startLoop();
  // loop->runInLoop(std::bind(print, loop));
  // CurrentThread::sleepUsec(500 * 1000);
  // }

  // {
  // // quit() before dtor
  // EventLoopThread thr3;
  // EventLoop* loop = thr3.startLoop();
  // loop->runInLoop(std::bind(quit, loop));
  // CurrentThread::sleepUsec(500 * 1000);
  // }
}

