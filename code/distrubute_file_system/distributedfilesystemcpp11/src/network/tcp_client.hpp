#ifndef DFS_TCP_CLIENT_HPP
#define DFS_TCP_CLIENT_HPP

#include <string>
#include <functional>

namespace dfs {

class TcpClient {
public:
    TcpClient(const std::string& host, int port);
    // 阻塞连接，成功返回 true
    bool connect();
    // 底层 fd，供 codec 使用
    int fd() const { return fd_; }
    void close();
    ~TcpClient();
private:
    std::string host_;
    int port_;
    int fd_;
};

} // namespace dfs

#endif
