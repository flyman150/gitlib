#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

class TcpClient
{
public:
    TcpClient(const std::string& ip, int port);
    ~TcpClient();

    // 禁止拷贝和赋值
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    // 启动客户端
    void start();

    // 停止客户端
    void stop();

    // 发送数据
    bool send(const std::string& data);

    // 设置连接状态回调
    void setConnectionCallback(std::function<void(bool)> callback);

    // 检查客户端状态
    bool isRunning() const;
    bool isConnected() const;

private:
    // 连接到服务器
    bool connect();

    // 重连线程函数
    void reconnectThread();

    // 关闭socket
    void closeSocket();

    // 通知连接状态变化
    void notifyConnectionChange(bool connected);

private:
    std::string server_ip_;
    int server_port_;
    int sock_fd_;
    std::atomic<bool> running_;
    bool connected_;
    std::thread reconnect_thread_;
    std::function<void(bool)> connection_callback_;
    mutable std::mutex mutex_;
};