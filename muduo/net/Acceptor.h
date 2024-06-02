// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <functional>

#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"

namespace muduo
{
namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
/// 接受新用户连接并分发连接给SubReactor（SubEventLoop）
/// Acceptor用于main EventLoop中，对服务器监听套接字fd及其相关方法进行
/// 封装（监听、接受连接、分发连接给SubEventLoop等）
///
/// 关心的是监听套接字的可读事件
/// 被动连接
/// Acceptor的生存周期由TcpServer控制
/// 整体过程：
///   Acceptor的acceptSocket_是监听socket，acceptChannel_用于观察此socket
///   的可读事件（必须先注册可读事件，也就是注册到Poller中），Poller::poll监听到
///   可读事件发生之后，将acceptChannel_加入到活动的Channel中，然后在loop.loop()
///   中调用acceptChannel_的handleEvent
///   因为是可读事件，因此最终调用到Acceptor::handleRead函数(在构造函数中就已经
///   注册了acceptChannel_的读回调函数为Acceptor::handleRead函数)
class Acceptor : noncopyable
{
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void listen();

  bool listening() const { return listening_; }

  // Deprecated, use the correct spelling one above.
  // Leave the wrong spelling here in case one needs to grep it for error messages.
  // bool listenning() const { return listening(); }

 private:
  // 处理可读事件（主要是新的客户连接到来）
  // 可读事件的注册由Channel来负责，当可读事件发生的时候
  // Channel的handleEvent会回调handleRead()
  void handleRead();

  // 监听套接字的fd由哪个EventLoop负责循环监听以及处理相应事件，
  // 其实这个EventLoop就是main EventLoop。
  EventLoop* loop_;
  Socket acceptSocket_;   // 这个是服务器监听套接字的文件描述符
  Channel acceptChannel_; // 这是个Channel类，把acceptSocket_及其感兴趣事件和事件对应的处理函数都封装进去。
  // TcpServer构造函数中将- TcpServer::newConnection( )函数注册给了这个成员变量。
  // 这个- TcpServer::newConnection函数的功能是公平的选择一个subEventLoop，并把
  // 已经接受的连接分发给这个subEventLoop。
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
  int idleFd_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ACCEPTOR_H
