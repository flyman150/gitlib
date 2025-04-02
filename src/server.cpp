#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <cstring>
#include <errno.h>
#include <algorithm>

class TcpServer {
public:
    explicit TcpServer(int port) : port_(port), running_(false), server_fd_(-1) {}
    
    ~TcpServer() {
        stop();
    }
    
    void start() {
        if (running_) return;
        
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            std::cerr << "创建socket失败: " << strerror(errno) << std::endl;
            return;
        }

        // 设置socket选项，允许地址重用
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "设置socket选项失败: " << strerror(errno) << std::endl;
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "绑定端口失败: " << strerror(errno) << std::endl;
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        if (listen(server_fd_, 10) < 0) {
            std::cerr << "监听失败: " << strerror(errno) << std::endl;
            close(server_fd_);
            server_fd_ = -1;
            return;
        }

        running_ = true;
        std::cout << "服务器启动成功，监听端口: " << port_ << std::endl;

        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            std::cout << "等待客户端连接..." << std::endl;
            int client_socket = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_socket < 0) {
                if (errno == EINTR) {  // 被信号中断
                    continue;
                }
                std::cerr << "接受连接失败: " << strerror(errno) << std::endl;
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "新客户端连接，IP: " << client_ip 
                      << ", 端口: " << ntohs(client_addr.sin_port) << std::endl;

            // 使用智能指针管理客户端线程
            auto client_thread = std::make_unique<std::thread>(&TcpServer::handleClient, this, client_socket);
            client_threads_.push_back(std::move(client_thread));
            
            // 清理已完成的线程
            cleanupFinishedThreads();
        }
    }

    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // 关闭服务器socket
        if (server_fd_ != -1) {
            close(server_fd_);
            server_fd_ = -1;
        }
        
        // 等待所有客户端线程结束
        for (auto& thread : client_threads_) {
            if (thread && thread->joinable()) {
                thread->join();
            }
        }
        client_threads_.clear();
        
        std::cout << "服务器已停止" << std::endl;
    }

private:
    void handleClient(int client_socket) {
        // 使用RAII方式管理客户端socket
        struct SocketGuard {
            int fd;
            SocketGuard(int fd) : fd(fd) {}
            ~SocketGuard() { if (fd != -1) close(fd); }
        } guard(client_socket);
        
        char buffer[1024];
        while (running_) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cout << "客户端主动断开连接" << std::endl;
                } else if (errno != EINTR) {  // 忽略被信号中断的情况
                    std::cerr << "接收数据失败: " << strerror(errno) << std::endl;
                }
                break;
            }
            buffer[bytes_read] = '\0';
            std::cout << "收到消息: " << buffer << std::endl;
            
            // 发送响应
            std::string response = "服务器已收到消息";
            if (send(client_socket, response.c_str(), response.length(), 0) < 0) {
                if (errno != EINTR) {  // 忽略被信号中断的情况
                    std::cerr << "发送响应失败: " << strerror(errno) << std::endl;
                }
                break;
            }
        }
        std::cout << "客户端连接已关闭" << std::endl;
    }
    
    void cleanupFinishedThreads() {
        client_threads_.erase(
            std::remove_if(
                client_threads_.begin(),
                client_threads_.end(),
                [](const auto& thread) {
                    return thread && !thread->joinable();
                }
            ),
            client_threads_.end()
        );
    }

    int server_fd_;
    int port_;
    std::atomic<bool> running_;
    std::vector<std::unique_ptr<std::thread>> client_threads_;
};

int main() {
    TcpServer server(8888);
    std::cout << "正在启动服务器..." << std::endl;
    server.start();
    return 0;
} 