// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <map>
#include <vector>

#include "muduo/base/Timestamp.h"
#include "muduo/net/EventLoop.h"

namespace muduo
{
namespace net
{

class Channel;

///
/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class Poller : noncopyable
{
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  // Poller的核心函数，当外部调用poll方法的时候，该方法底层是通过 epoll_wait 获取这个事件
  // 监听器上发生事件的fd及其对应发生的事件，我们知道每个fd都是由一个Channel封装的，通过哈希表
  // channels_可以根据fd找到封装这个fd的Channel。
  //
  // 将事件监听器监听到该fd发生的事件写进这个Chanel中的revents_成员变量中。然后把这个Channel
  // 装进一个activeChannels中（它是一个vector<Channel*>）
  // 这样，当外界调用完poll之后就可以拿到事件监听器的监听结果
  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void updateChannel(Channel* channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeChannel(Channel* channel) = 0;

  virtual bool hasChannel(Channel* channel) const;

  static Poller* newDefaultPoller(EventLoop* loop);

  void assertInLoopThread() const
  {
    ownerLoop_->assertInLoopThread();
  }

 protected:
  typedef std::map<int, Channel*> ChannelMap;
  // fd和Channel*的映射关系
  // Poller和Channel是关联关系，Poller不管理Channel的生存周期
  // 保管所有- 注册在你这个Poller上的Channel
  ChannelMap channels_;  

 private:
  EventLoop* ownerLoop_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_POLLER_H
