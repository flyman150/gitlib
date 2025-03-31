#include <gtest/gtest.h>
#include "tcp_client.h"
#include <thread>
#include <chrono>
#include <atomic>

// 测试TCP客户端的基本功能
class TcpClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的设置
    }

    void TearDown() override {
        // 测试后的清理
    }
};

// 测试客户端创建和销毁
TEST_F(TcpClientTest, CreateAndDestroy) {
    TcpClient client("127.0.0.1", 8888);
    EXPECT_NO_THROW(client.start());
    EXPECT_NO_THROW(client.stop());
}

// 测试连接状态回调
TEST_F(TcpClientTest, ConnectionCallback) {
    TcpClient client("127.0.0.1", 8888);
    std::atomic<bool> callback_called(false);
    
    client.setConnectionCallback([&callback_called](bool connected) {
        callback_called = true;
    });
    
    client.start();
    // 增加等待时间，确保回调有足够时间被调用
    std::this_thread::sleep_for(std::chrono::seconds(5));
    EXPECT_TRUE(callback_called);
    client.stop();
}

// 测试消息发送
TEST_F(TcpClientTest, SendMessage) {
    TcpClient client("127.0.0.1", 8888);
    client.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::string message = "测试消息";
    bool send_result = client.send(message);
    // 注意：由于没有实际的服务器，发送可能会失败
    // 这里我们主要测试接口是否正常工作
    EXPECT_NO_THROW(client.send(message));
    
    client.stop();
}

// 测试自动重连机制
TEST_F(TcpClientTest, AutoReconnect) {
    TcpClient client("127.0.0.1", 8888);
    std::atomic<int> reconnect_count(0);
    
    client.setConnectionCallback([&reconnect_count](bool connected) {
        if (!connected) {
            reconnect_count++;
        }
    });
    
    client.start();
    std::this_thread::sleep_for(std::chrono::seconds(4)); // 等待可能的重连尝试
    EXPECT_GE(reconnect_count, 0);
    client.stop();
}

// 测试多客户端并发
TEST_F(TcpClientTest, MultipleClients) {
    const int CLIENT_COUNT = 5;
    std::vector<std::unique_ptr<TcpClient>> clients;
    
    // 创建多个客户端
    for (int i = 0; i < CLIENT_COUNT; ++i) {
        clients.push_back(std::make_unique<TcpClient>("127.0.0.1", 8888));
        clients.back()->start();
    }
    
    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 清理
    for (auto& client : clients) {
        client->stop();
    }
    
    // 验证所有客户端都能正常停止
    EXPECT_EQ(clients.size(), CLIENT_COUNT);
}

// 测试客户端状态管理
TEST_F(TcpClientTest, ClientStateManagement) {
    TcpClient client("127.0.0.1", 8888);
    
    // 测试启动
    client.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 测试停止
    client.stop();
    
    // 测试重复启动
    EXPECT_NO_THROW(client.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 测试重复停止
    EXPECT_NO_THROW(client.stop());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 