#include "tcp_client.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include <algorithm>

TcpClient::TcpClient(const std::string& ip, int port)
    : server_ip_(ip)
    , server_port_(port)
    , sock_fd_(-1)
    , running_(false)
    , connected_(false) {
}

TcpClient::~TcpClient() {
    stop();
}

void TcpClient::start() {
    if (running_) return;
    
    running_ = true;
    connected_ = false;
    
    // 创建重连线程
    reconnect_thread_ = std::thread(&TcpClient::reconnectThread, this);
    
    // 尝试首次连接
    if (connect()) {
        connected_ = true;
        if (connection_callback_) {
            connection_callback_(true);
        }
    }
}

void TcpClient::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // 关闭socket
    if (sock_fd_ != -1) {
        close(sock_fd_);
        sock_fd_ = -1;
    }
    
    // 等待重连线程结束
    if (reconnect_thread_.joinable()) {
        reconnect_thread_.join();
    }
    
    connected_ = false;
    if (connection_callback_) {
        connection_callback_(false);
    }
}

bool TcpClient::connect() {
    if (sock_fd_ >= 0) {
        close(sock_fd_);
        sock_fd_ = -1;
    }
    
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_ == -1) {
        std::cerr << "创建socket失败: " << strerror(errno) << std::endl;
        return false;
    }

    // 设置socket选项
    int opt = 1;
    if (setsockopt(sock_fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
        std::cerr << "设置socket选项失败: " << strerror(errno) << std::endl;
        close(sock_fd_);
        sock_fd_ = -1;
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);
    server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());

    std::cout << "正在连接服务器 " << server_ip_ << ":" << server_port_ << std::endl;
    if (::connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "连接服务器失败: " << strerror(errno) << std::endl;
        close(sock_fd_);
        sock_fd_ = -1;
        return false;
    }

    std::cout << "成功连接到服务器" << std::endl;
    return true;
}

void TcpClient::reconnectThread() {
    while (running_) {
        if (!connected_) {
            std::cout << "尝试重新连接服务器..." << std::endl;
            if (connect()) {
                connected_ = true;
                if (connection_callback_) {
                    connection_callback_(true);
                }
                std::cout << "重连成功" << std::endl;
            } else {
                std::cout << "重连失败，3秒后重试" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool TcpClient::send(const std::string& data) {
    if (!connected_ || sock_fd_ == -1) {
        std::cerr << "未连接到服务器，无法发送数据" << std::endl;
        return false;
    }

    std::cout << "正在发送数据: " << data << std::endl;
    ssize_t sent = ::send(sock_fd_, data.c_str(), data.length(), 0);
    if (sent == -1) {
        if (errno != EINTR) {  // 忽略被信号中断的情况
            std::cerr << "发送数据失败: " << strerror(errno) << std::endl;
            connected_ = false;
            if (connection_callback_) {
                connection_callback_(false);
            }
            return false;
        }
    }

    std::cout << "数据发送成功" << std::endl;
    return true;
}

void TcpClient::setConnectionCallback(std::function<void(bool)> callback) {
    connection_callback_ = callback;
} 