#!/bin/bash

# test_build.sh
set -e

echo "开始内存泄漏测试..."

# 检查必要工具
command -v valgrind >/dev/null 2>&1 || { 
    echo "需要安装 valgrind"
    echo "在 Ubuntu 上可以使用: sudo apt-get install valgrind"
    exit 1 
}

# 确保项目已经构建
if [ ! -f "build/tcp_client" ]; then
    echo "未找到tcp_client程序，先运行 build.sh"
    mkdir -p build
    cd build
    cmake ..
    make
    cd ..
fi

# 清理测试文件
echo "清理测试文件..."
rm -f valgrind_*.log

# 运行客户端测试
echo "运行客户端内存泄漏测试..."
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file="valgrind_client.log" \
         ./build/tcp_client &

# 等待5秒后终止客户端
sleep 5
killall tcp_client

# 检查客户端测试结果
if grep -q "definitely lost: 0 bytes" "valgrind_client.log" && \
   grep -q "indirectly lost: 0 bytes" "valgrind_client.log"; then
    echo "✅ 客户端测试未检测到内存泄漏"
else
    echo "❌ 客户端测试检测到内存泄漏，详见 valgrind_client.log"
fi

echo "所有测试完成"