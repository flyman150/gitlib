#include "tcp_client.h"
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

// 全局变量用于控制所有客户端
std::atomic<bool> g_running(true);

// 客户端线程函数
void clientThread(const int client_id, const std::string& server_ip, const int server_port) {
    // 创建TCP客户端实例
    TcpClient client(server_ip, server_port);
    
    // 设置连接状态回调
    client.setConnectionCallback([client_id](bool connected) {
        std::cout << "客户端 " << client_id << " 连接状态: " 
                  << (connected ? "已连接" : "已断开") << std::endl;
    });
    
    // 启动客户端
    client.start();
    
    // 主循环
    while (g_running) {
        std::string message = "客户端 " + std::to_string(client_id) + " 发送的消息";
        if (client.send(message)) {
            std::cout << "客户端 " << client_id << " 消息发送成功" << std::endl;
        } else {
            std::cout << "客户端 " << client_id << " 消息发送失败" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // 停止客户端
    client.stop();
}

int main() {
    const int CLIENT_COUNT = 10;
    const std::string SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 8888;
    
    std::vector<std::thread> client_threads;
    
    // 创建并启动所有客户端线程
    for (int i = 0; i < CLIENT_COUNT; ++i) {
        client_threads.emplace_back(clientThread, i, SERVER_IP, SERVER_PORT);
    }
    
    // 等待用户输入来停止所有客户端
    std::cout << "按回车键停止所有客户端..." << std::endl;
    std::cin.get();
    
    // 设置停止标志
    g_running = false;
    
    // 等待所有客户端线程结束
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "所有客户端已停止" << std::endl;
    return 0;
} 