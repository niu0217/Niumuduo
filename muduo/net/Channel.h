// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Timestamp.h"

#include <functional>
#include <memory>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
/// 对IO事件的响应与封装 注册IO的可读可写等事件
class Channel : noncopyable
{
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  void setReadCallback(ReadEventCallback cb)
  { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb)
  { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb)
  { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb)
  { errorCallback_ = std::move(cb); }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  // 当事件监听器监听到某个文件描述符发生了什么事件，通过这个函数
  // 可以将这个文件描述符实际发生的事件封装进这个Channel中。
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  /// 将Channel中的文件描述符及其感兴趣事件注册到事件监听器上或从事件监听器上移除
  void enableReading() { events_ |= kReadEvent; update(); }
  void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  string reventsToString() const;
  string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  static string eventsToString(int fd, int ev);

  // 调用关系：调用EventLoop的updateChannel()
  //         ==>  再调用Poller的updateChannel(channel)
  // 将fd_的可读可写等事件注册到Poller中
  // 至此，一个fd对应一个Channel
  void update();
  // 让每个发生了事件的Channel调用自己保管的事件处理函数，每个Channel会根据自己文件
  // 描述符实际发生的事件（通过Channel中的revents_可知）和感兴趣的事件（通过Channel
  // 中的events_可知）来调用对应的事件处理函数
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  // 一个EventLoop可以对应多个Channel，但是一个Channel只能对应于一个EventLoop
  EventLoop* loop_;
  // Channel不拥有文件描述符，也就是说当Channel析构的时候，文件描述符并没有被close掉
  // Channel和文件描述符是关联关系，一个Channel有一个文件描述符
  // EventLoop和文件描述符也是关联关系，一个EventLoop有多个文件描述符
  // 那么文件描述符fd_是被谁管理的呢？答案是Socket类，它的生命周期由Socket类来管理
  // Socket类的析构函数会调用close来关闭文件描述符fd_
  const int  fd_;
  int        events_;  // 它关心的事件
  int        revents_; // it's the received event types of epoll or poll
  int        index_; // used by Poller.  表示在poll的事件数组中的序号
  bool       logHup_;

  // 当连接到来的时候，创建一个TcpConnection对象，立刻用shared_ptr来管理，引用计数为1
  // 在Channel中维护一个 std::weak_ptr<void> tie_; 将这个shared_ptr对象赋值给tie_
  // 引用计数仍然为1
  //
  // 当连接关闭的时候，在handleEvent中将tie_提升，得到一个shared_ptr对象，引用计数变为了2
  std::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;  // 是否处于处理事件中
  bool addedToLoop_;
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H

/*
  Channel类的作用：
    保姆；一个文件描述符会发生可读、可写、关闭、错误事件。当发生这些事件后，就需要调用相应的处理
    函数来处理。外部通过调用setReadCallback、setWriteCallback、setCloseCallback、
    setErrorCallback这四个函数可以将事件处理函数存放在Channel类中，当需要调用的时候
    就可以直接拿出来调用即可。
*/