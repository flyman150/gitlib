#!/bin/bash

# 检查 clang-format 是否安装
if ! command -v clang-format &> /dev/null; then
    echo "错误: clang-format 未安装。请先安装 clang-format。"
    exit 1
fi

# 格式化所有 C++ 文件
echo "开始格式化所有 C++ 文件..."

# 主目录下的文件
find . -maxdepth 1 -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;

# src 目录下的文件
if [ -d "src" ]; then
    find src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;
fi

# tests 目录下的文件
if [ -d "tests" ]; then
    find tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;
fi

# include 目录下的文件
if [ -d "include" ]; then
    find include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} \;
fi

echo "格式化完成！" 