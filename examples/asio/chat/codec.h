#ifndef MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
#define MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H

#include "muduo/base/Logging.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/Endian.h"
#include "muduo/net/TcpConnection.h"

class LengthHeaderCodec : muduo::noncopyable
{
 public:
  typedef std::function<void (const muduo::net::TcpConnectionPtr&,
                                const muduo::string& message,
                                muduo::Timestamp)> StringMessageCallback;

  explicit LengthHeaderCodec(const StringMessageCallback& cb)
    : messageCallback_(cb)
  {
  }

  /// 当TCP连接 conn 发送消息的时候，在这个函数中将字节流消息解码
  /// 字节流消息是存放在 buf 中
  /// 解码字节流消息之后就将它发送给所有连接到服务器的客户端
  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime)
  {
    // 这里不用if是因为有可能会收到多个消息，所以需要循环处理
    // 在这个循环中处理消息的 粘包问题
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
      // FIXME: use Buffer::peekInt32()
      const void* data = buf->peek();
      // 前四个字节保存的是这条消息的长度
      int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
      // 此时 len 保存的就是要处理消息的长度
      const int32_t len = muduo::net::sockets::networkToHost32(be32);
      if (len > 65536 || len < 0)
      {
        LOG_ERROR << "Invalid length " << len;
        conn->shutdown();  // FIXME: disable reading
        break;
      }
      else if (buf->readableBytes() >= len + kHeaderLen) // 达到一条完整的消息
      {
        /// 一条正确的消息举例：0x00,0x00,0x00,0x05,'h','e','l','l','o'
        buf->retrieve(kHeaderLen);  // 去掉四个字节的消息头部  该信息已经被保存在 len 中
        muduo::string message(buf->peek(), len);  // message此时保存的就是这条完整的消息了
        // messageCallback_ == ChatServer::onStringMessage
        // 将这条信息 message 发送给所有连接到 ChatServer的客户端
        // 如果是客户端的话 messageCallback_ 就对应客户端的回调函数
        messageCallback_(conn, message, receiveTime);
        buf->retrieve(len);  // 将message从buf中删除
      }
      else // 未达到一条完整的消息
      {
        /// 如果是错误的消息，比如：
        ///   0x00,0x00,0x00,0x08,'h','e','l','l'
        ///   0x00,0x00,0x00,0x08,'h','e','l','l',0x00,0x00,0x00,0x02,'h','e'
        /// 这时的处理办法有：（1）CR3校验；（2）空闲断开；
        /// 在这里我们没有考虑，而是直接退出了
        break;
      }
    }
  }

  // FIXME: TcpConnectionPtr
  void send(muduo::net::TcpConnection* conn,
            const muduo::StringPiece& message)
  {
    muduo::net::Buffer buf;
    buf.append(message.data(), message.size());  // 将 message 加入到应用层缓冲区buf中
    int32_t len = static_cast<int32_t>(message.size());
    // be32 保存的就是 message 的长度  
    // 表示形式为 4个字节的大端表示法(也就是网络字节序)
    int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    buf.prepend(&be32, sizeof be32); // 将4个字节的头部数据加到消息前面
    conn->send(&buf);  // 此时 buf 中保存的数据就是带有头部数据 发给客户端
  }

 private:
  // messageCallback_ 来自于 ChatServer::onStringMessage
  StringMessageCallback messageCallback_;
  const static size_t kHeaderLen = sizeof(int32_t);  // 4个字节
};

#endif  // MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
