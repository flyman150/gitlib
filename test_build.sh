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
if [ ! -f "build/tcp_client_test" ]; then
    echo "未找到测试程序，先运行 build.sh"
    ./build.sh
fi

# Valgrind测试函数
run_valgrind_test() {
    local test_name=$1
    local exec_file=$2
    echo "运行 $test_name 内存泄漏测试..."
    
    valgrind --leak-check=full \
             --show-leak-kinds=all \
             --track-origins=yes \
             --verbose \
             --log-file="valgrind_${test_name}.log" \
             $exec_file
             
    if grep -q "definitely lost: 0 bytes" "valgrind_${test_name}.log" && \
       grep -q "indirectly lost: 0 bytes" "valgrind_${test_name}.log"; then
        echo "✅ $test_name 未检测到内存泄漏"
    else
        echo "❌ $test_name 检测到内存泄漏，详见 valgrind_${test_name}.log"
        return 1
    fi
}

# 运行基本测试
run_valgrind_test "basic" "./build/tcp_client_test"

# 压力测试
echo "运行压力测试..."
for i in {1..5}; do
    ./build/tcp_client_test --gtest_repeat=10 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ 压力测试失败"
        exit 1
    fi
done
echo "✅ 压力测试通过"

# 清理测试文件
echo "清理测试文件..."
rm -f valgrind_*.log

echo "所有测试完成"