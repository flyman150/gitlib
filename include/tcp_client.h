#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

class TcpClient {
public:
    TcpClient(const std::string& ip, int port);
    ~TcpClient();

    // 启动客户端
    void start();
    // 停止客户端
    void stop();
    // 发送数据
    bool send(const std::string& data);
    // 设置连接状态回调
    void setConnectionCallback(std::function<void(bool)> callback);

private:
    // 连接服务器
    bool connect();
    // 重连线程
    void reconnectThread();

    std::string server_ip_;
    int server_port_;
    int sock_fd_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    std::thread reconnect_thread_;
    std::function<void(bool)> connection_callback_;
}; 