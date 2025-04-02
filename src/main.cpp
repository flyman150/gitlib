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
    std::string serverIp = "127.0.0.1";
    int serverPort = 8888;
    int clientCount = 10;
    int messageInterval = 2000;
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
                config.serverIp = argv[++i];
            }
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                config.serverPort = std::atoi(argv[++i]);
                if (config.serverPort <= 0 || config.serverPort > 65535) {
                    std::cerr << "错误：端口号必须在1-65535之间" << std::endl;
                    exit(1);
                }
            }
        } else if (arg == "-c" || arg == "--clients") {
            if (i + 1 < argc) {
                config.clientCount = std::atoi(argv[++i]);
                if (config.clientCount <= 0) {
                    std::cerr << "错误：客户端数量必须大于0" << std::endl;
                    exit(1);
                }
            }
        } else if (arg == "-i" || arg == "--interval") {
            if (i + 1 < argc) {
                config.messageInterval = std::atoi(argv[++i]);
                if (config.messageInterval < 100) {
                    std::cerr << "错误：消息间隔必须大于100毫秒" << std::endl;
                    exit(1);
                }
            }
        }
    }
    
    return config;
}

// 全局变量用于控制所有客户端
std::atomic<bool> gRunning(true);

// 客户端线程函数
void clientThread(const int clientId, 
                 const std::string& serverIp, 
                 const int serverPort,
                 const int messageInterval) {
    // 创建TCP客户端实例
    TcpClient client(serverIp, serverPort);
    
    // 设置连接状态回调
    client.setConnectionCallback([clientId](bool connected) {
        std::cout << "客户端 " << clientId << " 连接状态: " 
                  << (connected ? "已连接" : "已断开") << std::endl;
    });
    
    // 启动客户端
    client.start();
    
    // 主循环
    while (gRunning) {
        std::string message = "客户端 " + std::to_string(clientId) + " 发送的消息";
        if (client.send(message)) {
            std::cout << "客户端 " << clientId << " 消息发送成功" << std::endl;
        } else {
            std::cout << "客户端 " << clientId << " 消息发送失败" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(messageInterval));
    }
    
    // 停止客户端
    client.stop();
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    ClientConfig config = parseArguments(argc, argv);
    
    std::cout << "启动配置:\n"
              << "服务器IP: " << config.serverIp << "\n"
              << "服务器端口: " << config.serverPort << "\n"
              << "客户端数量: " << config.clientCount << "\n"
              << "消息发送间隔: " << config.messageInterval << "ms\n"
              << std::endl;
    
    std::vector<std::thread> clientThreads;
    
    // 创建并启动所有客户端线程
    for (int i = 0; i < config.clientCount; ++i) {
        clientThreads.emplace_back(clientThread, i, 
                                  config.serverIp, 
                                  config.serverPort,
                                  config.messageInterval);
    }
    
    // 等待用户输入来停止所有客户端
    std::cout << "按回车键停止所有客户端..." << std::endl;
    std::cin.get();
    
    // 设置停止标志
    gRunning = false;
    
    // 等待所有客户端线程结束
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "所有客户端已停止" << std::endl;
    return 0;
} 