#!/bin/bash

# 设置错误时退出
set -e

echo "开始构建项目..."

# 检查是否安装了必要的工具
command -v cmake >/dev/null 2>&1 || { echo "需要安装 cmake"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "需要安装 make"; exit 1; }
command -v pkg-config >/dev/null 2>&1 || { echo "需要安装 pkg-config"; echo "在 Ubuntu 上可以使用: sudo apt-get install pkg-config"; exit 1; }

# 检查是否安装了 Google Test
if ! pkg-config --exists gtest; then
    echo "需要安装 Google Test"
    echo "在 Ubuntu 上可以使用: sudo apt-get install libgtest-dev"
    echo "安装后需要手动编译 Google Test:"
    echo "cd /usr/src/gtest"
    echo "sudo cmake CMakeLists.txt"
    echo "sudo make"
    echo "sudo cp lib/*.a /usr/lib"
    exit 1
fi

# 获取CPU核心数
CPU_CORES=$(nproc)
echo "检测到CPU核心数: $CPU_CORES"

# 清理旧的构建文件
echo "清理旧的构建文件..."
rm -rf build
mkdir -p build
cd build

# 生成构建文件，启用多线程编译
echo "生成构建文件..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      -DCMAKE_C_FLAGS="-O3 -march=native" \
      ..

# 编译项目，使用所有CPU核心
echo "开始多线程编译（使用 $CPU_CORES 个线程）..."
make -j$CPU_CORES

echo "构建完成！"
echo "可执行文件位置："
echo "- 服务器: build/tcp_server"
echo "- 客户端: build/tcp_client"
echo "- 测试程序: build/tcp_client_test"

# 提示如何运行
echo -e "\n运行说明："
echo "1. 启动服务器："
echo "   ./build/tcp_server"
echo "2. 运行客户端："
echo "   ./build/tcp_client"
echo "3. 运行测试："
echo "   ./build/tcp_client_test"

# 重置终端设置
echo -e "\033[0m"  # 重置所有属性 