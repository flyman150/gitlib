cmake_minimum_required(VERSION 3.10)
project(tcp_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置调试模式和编译标志
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 添加include目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加可执行文件
add_executable(tcp_client src/main.cpp src/tcp_client.cpp)
add_executable(tcp_server src/server.cpp)

# 添加测试
enable_testing()
add_executable(tcp_client_test 
    src/tcp_client.cpp
    tests/test_tcp_client.cpp
)

# 添加测试依赖
find_package(GTest REQUIRED)
target_link_libraries(tcp_client_test GTest::GTest GTest::Main pthread)

# 添加测试到CTest
add_test(NAME tcp_client_test COMMAND tcp_client_test)