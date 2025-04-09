#include <string.h>        // 用于 strerror
#include <sys/resource.h>  // 用于设置core dump
#include <sys/time.h>
#include <unistd.h>  // 用于获取进程ID

#include <atomic>
#include <cerrno>  // 用于错误处理
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tcp_client.h"

void printUsage(const char* programName)
{
    std::cout << "用法: " << programName << " [选项]\n"
              << "选项:\n"
              << "  -h, --help                显示帮助信息\n"
              << "  -s, --server <IP>         服务器IP地址 (默认: 127.0.0.1)\n"
              << "  -p, --port <端口>         服务器端口 (默认: 8888)\n"
              << "  -c, --clients <数量>      客户端数量 (默认: 10)\n"
              << "  -i, --interval <毫秒>     发送消息间隔 (默认: 2000)\n"
              << std::endl;
}

struct ClientConfig
{
    std::string serverIp = "127.0.0.1";
    int serverPort = 8888;
    int clientCount = 10;
    int messageInterval = 2000;
};

ClientConfig parseArguments(int argc, char* argv[])
{
    ClientConfig config;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "-s" || arg == "--server")
        {
            if (i + 1 < argc)
            {
                config.serverIp = argv[++i];
            }
        }
        else if (arg == "-p" || arg == "--port")
        {
            if (i + 1 < argc)
            {
                config.serverPort = std::atoi(argv[++i]);
                if (config.serverPort <= 0 || config.serverPort > 65535)
                {
                    std::cerr << "错误：端口号必须在1-65535之间" << std::endl;
                    exit(1);
                }
            }
        }
        else if (arg == "-c" || arg == "--clients")
        {
            if (i + 1 < argc)
            {
                config.clientCount = std::atoi(argv[++i]);
                if (config.clientCount <= 0)
                {
                    std::cerr << "错误：客户端数量必须大于0" << std::endl;
                    exit(1);
                }
            }
        }
        else if (arg == "-i" || arg == "--interval")
        {
            if (i + 1 < argc)
            {
                config.messageInterval = std::atoi(argv[++i]);
                if (config.messageInterval < 100)
                {
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
void clientThread(const int clientId, const std::string& serverIp, const int serverPort,
                  const int messageInterval)
{
    try
    {
        // 创建TCP客户端实例
        auto client = std::make_unique<TcpClient>(serverIp, serverPort);

        // 设置连接状态回调
        client->setConnectionCallback(
            [clientId](bool connected)
            {
                std::cout << "客户端 " << clientId
                          << " 连接状态: " << (connected ? "已连接" : "已断开") << std::endl;
            });

        // 启动客户端
        client->start();

        // 主循环
        while (gRunning)
        {
            if (client->isConnected())
            {
                std::string message = "客户端 " + std::to_string(clientId) + " 发送的消息";
                if (client->send(message))
                {
                    std::cout << "客户端 " << clientId << " 消息发送成功" << std::endl;
                }
                else
                {
                    std::cout << "客户端 " << clientId << " 消息发送失败" << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(messageInterval));
        }

        // 停止客户端
        client->stop();
    }
    catch (const std::exception& e)
    {
        std::cerr << "客户端 " << clientId << " 发生异常: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    // 设置 core dump 文件格式和大小限制
    std::string core_pattern = "./core_%e.%p";  // 修改为直接指定本地目录
    std::string cmd = "echo \"" + core_pattern + "\" | sudo tee /proc/sys/kernel/core_pattern";
    if (system(cmd.c_str()) != 0)
    {
        std::cerr << "设置 core pattern 失败，请确保有 sudo 权限" << std::endl;
    }

    // 允许生成无限制大小的 core dump 文件
    struct rlimit core_limits;
    core_limits.rlim_cur = RLIM_INFINITY;
    core_limits.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &core_limits) != 0)
    {
        std::cerr << "设置 core dump 大小限制失败: " << strerror(errno) << std::endl;
    }

    // 确保当前目录可写
    if (access(".", W_OK) != 0)
    {
        std::cerr << "警告：当前目录不可写，core dump 可能无法生成" << std::endl;
    }

    try
    {
        // 解析命令行参数
        ClientConfig config = parseArguments(argc, argv);

        std::cout << "启动配置:\n"
                  << "服务器IP: " << config.serverIp << "\n"
                  << "服务器端口: " << config.serverPort << "\n"
                  << "客户端数量: " << config.clientCount << "\n"
                  << "消息发送间隔: " << config.messageInterval << "ms\n"
                  << std::endl;

        std::vector<std::thread> clientThreads;
        clientThreads.reserve(config.clientCount);  // 预分配空间

        // 创建并启动所有客户端线程
        for (int i = 0; i < config.clientCount; ++i)
        {
            clientThreads.emplace_back(clientThread, i, config.serverIp, config.serverPort,
                                       config.messageInterval);
        }

        // 等待用户输入来停止所有客户端
        std::cout << "按回车键停止所有客户端..." << std::endl;
        std::cin.get();

        // 设置停止标志
        gRunning = false;

        // 等待所有客户端线程结束
        for (auto& thread : clientThreads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        std::cout << "所有客户端已停止" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "程序发生异常: " << e.what() << std::endl;
        return 1;
    }
}