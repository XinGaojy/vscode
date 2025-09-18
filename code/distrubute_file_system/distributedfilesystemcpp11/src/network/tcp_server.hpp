#ifndef DFS_TCP_SERVER_HPP
#define DFS_TCP_SERVER_HPP

#include <string>
#include <functional>
#include <memory>

namespace dfs {

class TcpServer {
public:
  typedef std::function<void(int fd, const std::string& addr)> OnConnect;
  TcpServer(const std::string& host, int port);
  void serve(OnConnect cb);
private:
  std::string host_;
  int port_;
  int listen_fd_;
};

} // namespace dfs

#endif
