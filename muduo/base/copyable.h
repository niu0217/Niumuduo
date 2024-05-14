#ifndef MUDUO_BASE_COPYABLE_H
#define MUDUO_BASE_COPYABLE_H

namespace muduo
{

// 值语义：可以拷贝的，拷贝之后，与原对象脱离关系
// 对象语义： 不可拷贝；或者可以拷贝，但是拷贝之后与原对象仍然存在一定的关系，比如共享底层资源

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
 protected:
  copyable() = default;
  ~copyable() = default;
};

}  // namespace muduo

#endif  // MUDUO_BASE_COPYABLE_H
