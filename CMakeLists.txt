cmake_minimum_required(VERSION 2.6)

project(muduo C CXX)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# only build examples if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL "muduo")
  option(MUDUO_BUILD_EXAMPLES "Build Muduo examples" ON)
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror # 把警告当成错误
 -Wconversion # 一些可能改变值的隐式转换，给出警告
 -Wno-unused-parameter # 函数中出现未使用的参数，不给出警告
 -Wold-style-cast # C风格的转换，给出警告
 -Woverloaded-virtual # 如果函数的声明隐藏住了基类的虚函数，就给出警告
 -Wpointer-arith # 对函数指针或者void*类型的指针进行算术操作时给出警告
 -Wshadow # 当一个局部变量覆盖住了另一个局部变量或者全局变量时，给出警告
 -Wwrite-strings # 规定字符串常量的类型为const char[length],把这样的地址复制给non-const char*指针会产生警告
 -march=native # 指定CPU体系结构为本地平台
 # -MMD
 -std=c++11
 -rdynamic
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(Boost REQUIRED)
find_package(Protobuf)
find_package(CURL)
find_package(ZLIB)
find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
find_path(MHD_INCLUDE_DIR microhttpd.h)
find_library(MHD_LIBRARY NAMES microhttpd)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
find_library(BOOSTSYSTEM_LIBRARY NAMES boost_system)
find_path(TCMALLOC_INCLUDE_DIR gperftools/heap-profiler.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)
find_path(GD_INCLUDE_DIR gd.h)
find_library(GD_LIBRARY NAMES gd)
find_program(THRIFT_COMPILER thrift)
find_path(THRIFT_INCLUDE_DIR thrift)
find_library(THRIFT_LIBRARY NAMES thrift)

if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
  message(STATUS "found cares")
endif()
if(CURL_FOUND)
  message(STATUS "found curl")
endif()
if(PROTOBUF_FOUND)
  message(STATUS "found protobuf")
endif()
if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  message(STATUS "found tcmalloc")
endif()
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  message(STATUS "found hiredis")
endif()
if(GD_INCLUDE_DIR AND GD_LIBRARY)
  message(STATUS "found gd")
endif()
if(THRIFT_COMPILER AND THRIFT_INCLUDE_DIR AND THRIFT_LIBRARY)
  message(STATUS "found thrift")
endif()

include_directories(${Boost_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(muduo/base)
add_subdirectory(muduo/net)

if(MUDUO_BUILD_EXAMPLES)
  add_subdirectory(contrib)
  add_subdirectory(examples)
else()
  if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
    add_subdirectory(examples/cdns)
  endif()
endif()

