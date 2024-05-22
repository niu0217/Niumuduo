// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "muduo/base/Mutex.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Channel.h"

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
/// 管理所有的定时器
class TimerQueue : noncopyable
{
 public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(TimerCallback cb,
                   Timestamp when,
                   double interval);

  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  typedef std::pair<Timestamp, Timer*> Entry;  // 为了保证key的唯一性才这样设计
  typedef std::set<Entry> TimerList;  // 保证了定时器的唯一性同时保持高效的增加和删除效率
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  // ActiveTimerSet和TimerList保存的是相同的东西，只不过
  // TimerList是按照时间排序；ActiveTimerSet是按照地址排序；
  typedef std::set<ActiveTimer> ActiveTimerSet;

  // 下面两个函数只会在其所属的IO线程中调用，因此不必加锁
  // 服务器性能杀手之一是锁竞争，所以要尽可能少用锁
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void handleRead();
  // move out all expired timers
  // 返回超时的定时器列表
  std::vector<Entry> getExpired(Timestamp now);
  // 重置超时的定时器列表
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;
  // timerfdChannel_和TimerQueue是组合关系
  // TimerQueue负责管理timerfdChannel_的生存周期
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_; // 按到期时间排序

  // for cancel()
  // timers_和activeTimers_保存的是相同的数据
  // timers_是按到期时间排序；activeTimers_是按照对象地址排序
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */
  ActiveTimerSet cancelingTimers_;  // 保存的是被取消的定时器
};

}  // namespace net
}  // namespace muduo
#endif  // MUDUO_NET_TIMERQUEUE_H
