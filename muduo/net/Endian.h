// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
// 封装了字节序转换函数

// Points：
//   大端：把最高有效字节（MSB, Most Significant Byte）存放在内存的最低地址，即在内存中高位字节在前
//   小端：把最低有效字节（LSB, Least Significant Byte）存放在内存的最低地址，即在内存中低位字节在前
//   大端也叫网络字节序；主机字节序有大端/小端

#ifndef MUDUO_NET_ENDIAN_H
#define MUDUO_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace muduo
{
namespace net
{
namespace sockets
{

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

/// 注意：
///    htobe64这些函数不是POSIX中的函数，只适用于Linux
///    使用类似htobe32这种函数就没有考虑跨平台的情况，也就是不可移植到Windows平台
inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64); // be 代表Big Endian 大端字节序也就是网络字节序
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htobe32(host32); // 如果使用POSIX标准中的函数就是htonl
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htobe16(host16); // 如果使用POSIX标准中的函数就是htons
}

inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return be32toh(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
  return be16toh(net16);
}

#pragma GCC diagnostic pop

}  // namespace sockets
}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_ENDIAN_H
