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

class TcpServer {
public:
    TcpServer(int port) : port_(port), running_(false) {}
    
    void start() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            std::cerr << "创建socket失败: " << strerror(errno) << std::endl;
            return;
        }

        // 设置socket选项，允许地址重用
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "设置socket选项失败: " << strerror(errno) << std::endl;
            return;
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "绑定端口失败: " << strerror(errno) << std::endl;
            return;
        }

        if (listen(server_fd_, 10) < 0) {
            std::cerr << "监听失败: " << strerror(errno) << std::endl;
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
                std::cerr << "接受连接失败: " << strerror(errno) << std::endl;
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "新客户端连接，IP: " << client_ip 
                      << ", 端口: " << ntohs(client_addr.sin_port) << std::endl;

            client_threads_.emplace_back(&TcpServer::handleClient, this, client_socket);
        }
    }

    void stop() {
        running_ = false;
        if (server_fd_ != -1) {
            close(server_fd_);
        }
        
        for (auto& thread : client_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        std::cout << "服务器已停止" << std::endl;
    }

private:
    void handleClient(int client_socket) {
        char buffer[1024];
        while (running_) {
            int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cout << "客户端主动断开连接" << std::endl;
                } else {
                    std::cerr << "接收数据失败: " << strerror(errno) << std::endl;
                }
                break;
            }
            buffer[bytes_read] = '\0';
            std::cout << "收到消息: " << buffer << std::endl;
            
            // 发送响应
            std::string response = "服务器已收到消息";
            if (send(client_socket, response.c_str(), response.length(), 0) < 0) {
                std::cerr << "发送响应失败: " << strerror(errno) << std::endl;
                break;
            }
        }
        close(client_socket);
        std::cout << "客户端连接已关闭" << std::endl;
    }

    int server_fd_;
    int port_;
    std::atomic<bool> running_;
    std::vector<std::thread> client_threads_;
};

int main() {
    TcpServer server(8888);
    std::cout << "正在启动服务器..." << std::endl;
    server.start();
    return 0;
} 