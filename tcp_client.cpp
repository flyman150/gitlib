#include "tcp_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <chrono>

TcpClient::TcpClient(const std::vector<std::string>& server_ips, 
                     const std::vector<int>& server_ports,
                     int reconnect_interval)
    : server_ips_(server_ips)
    , server_ports_(server_ports)
    , current_server_index_(0)
    , reconnect_interval_(reconnect_interval)
    , socket_fd_(-1)
    , running_(false)
    , connected_(false) {
    if (server_ips_.size() != server_ports_.size()) {
        throw std::runtime_error("服务器IP和端口数量不匹配");
    }
}

TcpClient::~TcpClient() {
    stop();
}

void TcpClient::start() {
    if (running_) return;
    
    running_ = true;
    std::thread(&TcpClient::reconnectThread, this).detach();
    
    // 尝试首次连接
    connectToServer();
}

void TcpClient::stop() {
    if (!running_) return;
    
    running_ = false;
    disconnect();
}

bool TcpClient::sendMessage(const std::string& message) {
    if (!connected_) {
        std::cout << "未连接到服务器，无法发送消息" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    ssize_t sent = send(socket_fd_, message.c_str(), message.length(), 0);
    return sent == static_cast<ssize_t>(message.length());
}

void TcpClient::setConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

bool TcpClient::isConnected() const {
    return connected_;
}

std::string TcpClient::getCurrentServer() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (server_ips_.empty()) return "";
    return server_ips_[current_server_index_] + ":" + 
           std::to_string(server_ports_[current_server_index_]);
}

bool TcpClient::connectToServer() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取当前服务器信息
    auto [ip, port] = getNextServer();
    
    // 创建socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cout << "创建socket失败" << std::endl;
        return false;
    }
    
    // 设置服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cout << "无效的IP地址: " << ip << std::endl;
        close(socket_fd_);
        return false;
    }
    
    // 尝试连接
    std::cout << "正在连接到服务器: " << ip << ":" << port << std::endl;
    if (connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cout << "连接失败: " << ip << ":" << port << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    connected_ = true;
    if (connection_callback_) {
        connection_callback_(true);
    }
    
    std::cout << "成功连接到服务器: " << ip << ":" << port << std::endl;
    return true;
}

void TcpClient::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
    if (connection_callback_) {
        connection_callback_(false);
    }
}

void TcpClient::reconnectThread() {
    while (running_) {
        if (!connected_) {
            if (!connectToServer()) {
                switchToNextServer();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_interval_));
    }
}

void TcpClient::switchToNextServer() {
    std::lock_guard<std::mutex> lock(mutex_);
    current_server_index_ = (current_server_index_ + 1) % server_ips_.size();
    std::cout << "切换到下一个服务器..." << std::endl;
}

std::pair<std::string, int> TcpClient::getNextServer() {
    return {server_ips_[current_server_index_], server_ports_[current_server_index_]};
} 