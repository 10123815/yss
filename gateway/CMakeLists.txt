# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
# 项目信息
project (gateway)
# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
FILE(GLOB SOURCE_1 "${CMAKE_SOURCE_DIR}/src/*.cc") 
# c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# 指定生成目标
add_executable(../bin/gateway_server ${SOURCE_1})
# thread
target_link_libraries(../bin/gateway_server pthread rt)