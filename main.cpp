#include "tcp_client.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 定义服务器列表
    std::vector<std::string> server_ips = {
        "www.baidu.com",
        "www.github.com"
    };
    std::vector<int> server_ports = {
        80,  // HTTP端口
        443  // HTTPS端口
    };
    
    // 创建客户端实例
    TcpClient client(server_ips, server_ports, 3000);
    
    // 设置连接状态回调
    client.setConnectionCallback([](bool connected) {
        std::cout << "连接状态变化: " << (connected ? "已连接" : "已断开") << std::endl;
    });
    
    // 启动客户端
    std::cout << "启动客户端..." << std::endl;
    client.start();
    
    // 主循环
    while (true) {
        if (client.isConnected()) {
            std::string message = "Hello from client to " + client.getCurrentServer();
            if (client.sendMessage(message)) {
                std::cout << "消息发送成功" << std::endl;
            } else {
                std::cout << "消息发送失败" << std::endl;
            }
        }
        
        // 等待2秒后发送下一条消息
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    return 0;
} 