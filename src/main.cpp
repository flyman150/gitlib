#include "tcp_client.h"
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdlib>
#include <string>

void printUsage(const char* programName) {
    std::cout << "用法: " << programName << " [选项]\n"
              << "选项:\n"
              << "  -h, --help                显示帮助信息\n"
              << "  -s, --server <IP>         服务器IP地址 (默认: 127.0.0.1)\n"
              << "  -p, --port <端口>         服务器端口 (默认: 8888)\n"
              << "  -c, --clients <数量>      客户端数量 (默认: 10)\n"
              << "  -i, --interval <毫秒>     发送消息间隔 (默认: 2000)\n"
              << std::endl;
}

struct ClientConfig {
    std::string server_ip = "127.0.0.1";
    int server_port = 8888;
    int client_count = 10;
    int message_interval = 2000;
};

ClientConfig parseArguments(int argc, char* argv[]) {
    ClientConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "-s" || arg == "--server") {
            if (i + 1 < argc) {
                config.server_ip = argv[++i];
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.server_port = std::atoi(argv[++i]);
                if (config.server_port <= 0 || config.server_port > 65535) {
                    std::cerr << "错误：端口号必须在1-65535之间" << std::endl;
                    exit(1);
                }
            }
        } else if (arg == "-c" || arg == "--clients") {
            if (i + 1 < argc) {
                config.client_count = std::atoi(argv[++i]);
                if (config.client_count <= 0) {
                    std::cerr << "错误：客户端数量必须大于0" << std::endl;
                    exit(1);
                }
            }
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 < argc) {
                config.message_interval = std::atoi(argv[++i]);
                if (config.message_interval < 100) {
                    std::cerr << "错误：消息间隔必须大于100毫秒" << std::endl;
                    exit(1);
                }
            }
        }
    }
    
    return config;
}

// 全局变量用于控制所有客户端
std::atomic<bool> g_running(true);

// 客户端线程函数
void clientThread(const int client_id, 
                 const std::string& server_ip, 
                 const int server_port,
                 const int message_interval) {
    // 创建TCP客户端实例
    std::vector<std::string> server_ips = {server_ip};
    std::vector<int> server_ports = {server_port};
    TcpClient client(server_ips, server_ports, 3000);
    
    // 设置连接状态回调
    client.setConnectionCallback([client_id](bool connected) {
        std::cout << "客户端 " << client_id << " 连接状态: " 
                  << (connected ? "已连接" : "已断开") << std::endl;
    });
    
    // 启动客户端
    client.start();
    
    // 主循环
    while (g_running) {
        if (client.isConnected()) {
            std::string message = "客户端 " + std::to_string(client_id) + " 发送的消息";
            if (client.sendMessage(message)) {
                std::cout << "客户端 " << client_id << " 消息发送成功" << std::endl;
            } else {
                std::cout << "客户端 " << client_id << " 消息发送失败" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(message_interval));
    }
    
    // 停止客户端
    client.stop();
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    ClientConfig config = parseArguments(argc, argv);
    
    std::cout << "启动配置:\n"
              << "服务器IP: " << config.server_ip << "\n"
              << "服务器端口: " << config.server_port << "\n"
              << "客户端数量: " << config.client_count << "\n"
              << "消息发送间隔: " << config.message_interval << "ms\n"
              << std::endl;
    
    std::vector<std::thread> client_threads;
    
    // 创建并启动所有客户端线程
    for (int i = 0; i < config.client_count; ++i) {
        client_threads.emplace_back(clientThread, i, 
                                  config.server_ip, 
                                  config.server_port,
                                  config.message_interval);
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