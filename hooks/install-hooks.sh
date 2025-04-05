#!/bin/bash

# 检查是否安装了必要的工具
check_dependencies() {
    if ! command -v cppcheck &> /dev/null; then
        echo "错误: 未安装 cppcheck"
        echo "Ubuntu/Debian 系统可以使用以下命令安装:"
        echo "sudo apt-get install cppcheck"
        exit 1
    fi
}

# 安装 git 钩子
install_hooks() {
    echo "开始安装 Git 钩子..."
    
    # 获取脚本所在目录的绝对路径
    SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    GIT_DIR="$( git rev-parse --git-dir )"
    
    # 创建 hooks 目录（如果不存在）
    mkdir -p "${GIT_DIR}/hooks"
    
    # 创建符号链接到 .git/hooks 目录
    ln -sf "$SCRIPT_DIR/pre-commit" "${GIT_DIR}/hooks/pre-commit"
    
    # 设置执行权限
    chmod +x "$SCRIPT_DIR/pre-commit"
    chmod +x "${GIT_DIR}/hooks/pre-commit"
    
    echo "✅ Git 钩子安装完成"
}

# 主函数
main() {
    check_dependencies
    install_hooks
}

# 运行主函数
main 