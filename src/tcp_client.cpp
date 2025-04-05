#include "tcp_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>

class SocketGuard
{
public:
    explicit SocketGuard(int& sock) : sock_(sock) {}
    ~SocketGuard()
    {
        if (sock_ >= 0)
        {
            close(sock_);
            sock_ = -1;
        }
    }

private:
    int& sock_;
};

TcpClient::TcpClient(const std::string& ip, int port)
    : server_ip_(ip), server_port_(port), sock_fd_(-1), running_(false), connected_(false)
{
}

TcpClient::~TcpClient()
{
    stop();
}

void TcpClient::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_)
        return;

    running_ = true;
    connected_ = false;

    // 创建重连线程
    reconnect_thread_ = std::thread(&TcpClient::reconnectThread, this);

    // 尝试首次连接
    if (connect())
    {
        connected_ = true;
        notifyConnectionChange(true);
    }
}

void TcpClient::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_)
            return;
        running_ = false;
    }

    // 关闭socket
    closeSocket();

    // 等待重连线程结束
    if (reconnect_thread_.joinable())
    {
        reconnect_thread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = false;
        notifyConnectionChange(false);
    }
}

void TcpClient::closeSocket()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (sock_fd_ != -1)
    {
        close(sock_fd_);
        sock_fd_ = -1;
    }
}

bool TcpClient::connect()
{
    std::lock_guard<std::mutex> lock(mutex_);

    closeSocket();

    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_ == -1)
    {
        std::cerr << "创建socket失败: " << strerror(errno) << std::endl;
        return false;
    }

    SocketGuard guard(sock_fd_);  // RAII: 自动管理socket资源

    // 设置非阻塞模式
    int flags = fcntl(sock_fd_, F_GETFL, 0);
    fcntl(sock_fd_, F_SETFL, flags | O_NONBLOCK);

    // 设置socket选项
    int opt = 1;
    if (setsockopt(sock_fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "设置socket选项失败: " << strerror(errno) << std::endl;
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port_);
    server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());

    // 非阻塞连接
    int ret = ::connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        if (errno != EINPROGRESS)
        {
            std::cerr << "连接服务器失败: " << strerror(errno) << std::endl;
            return false;
        }

        // 等待连接完成
        struct pollfd pfd;
        pfd.fd = sock_fd_;
        pfd.events = POLLOUT;

        ret = poll(&pfd, 1, 5000);  // 5秒超时
        if (ret <= 0)
        {
            std::cerr << "连接超时或错误" << std::endl;
            return false;
        }

        // 检查连接是否成功
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sock_fd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
        {
            std::cerr << "连接失败: " << strerror(error) << std::endl;
            return false;
        }
    }

    // 恢复阻塞模式
    fcntl(sock_fd_, F_SETFL, flags);

    std::cout << "成功连接到服务器" << std::endl;
    return true;
}

void TcpClient::reconnectThread()
{
    while (isRunning())
    {
        if (!isConnected())
        {
            std::cout << "尝试重新连接服务器..." << std::endl;
            if (connect())
            {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    connected_ = true;
                }
                notifyConnectionChange(true);
                std::cout << "重连成功" << std::endl;
            }
            else
            {
                std::cout << "重连失败，3秒后重试" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool TcpClient::send(const std::string& data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_ || sock_fd_ == -1)
    {
        std::cerr << "未连接到服务器，无法发送数据" << std::endl;
        return false;
    }

    std::cout << "正在发送数据: " << data << std::endl;

    size_t total_sent = 0;
    while (total_sent < data.length())
    {
        ssize_t sent =
            ::send(sock_fd_, data.c_str() + total_sent, data.length() - total_sent, MSG_NOSIGNAL);
        if (sent == -1)
        {
            if (errno == EINTR)
                continue;  // 重试被中断的系统调用
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 等待socket可写
                struct pollfd pfd;
                pfd.fd = sock_fd_;
                pfd.events = POLLOUT;
                if (poll(&pfd, 1, 1000) > 0)
                    continue;  // 1秒超时
            }
            std::cerr << "发送数据失败: " << strerror(errno) << std::endl;
            connected_ = false;
            notifyConnectionChange(false);
            return false;
        }
        total_sent += sent;
    }

    std::cout << "数据发送成功" << std::endl;
    return true;
}

void TcpClient::setConnectionCallback(std::function<void(bool)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connection_callback_ = std::move(callback);
}

bool TcpClient::isRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

bool TcpClient::isConnected() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_;
}

void TcpClient::notifyConnectionChange(bool connected)
{
    if (connection_callback_)
    {
        try
        {
            connection_callback_(connected);
        }
        catch (const std::exception& e)
        {
            std::cerr << "连接回调异常: " << e.what() << std::endl;
        }
    }
}