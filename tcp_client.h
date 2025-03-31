#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

class TcpClient {
public:
    // 连接状态回调函数类型
    using ConnectionCallback = std::function<void(bool)>;
    
    // 构造函数
    TcpClient(const std::vector<std::string>& server_ips, 
              const std::vector<int>& server_ports,
              int reconnect_interval = 3000);
    
    // 析构函数
    ~TcpClient();
    
    // 启动客户端
    void start();
    
    // 停止客户端
    void stop();
    
    // 发送消息
    bool sendMessage(const std::string& message);
    
    // 设置连接状态回调
    void setConnectionCallback(ConnectionCallback callback);
    
    // 获取当前连接状态
    bool isConnected() const;
    
    // 获取当前连接的服务器信息
    std::string getCurrentServer() const;

private:
    // 连接服务器
    bool connectToServer();
    
    // 断开连接
    void disconnect();
    
    // 重连线程函数
    void reconnectThread();
    
    // 切换到下一个服务器
    void switchToNextServer();
    
    // 获取下一个服务器信息
    std::pair<std::string, int> getNextServer();

    std::vector<std::string> server_ips_;
    std::vector<int> server_ports_;
    int current_server_index_;
    int reconnect_interval_;
    int socket_fd_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    ConnectionCallback connection_callback_;
    mutable std::mutex mutex_;
};

#endif // TCP_CLIENT_H 