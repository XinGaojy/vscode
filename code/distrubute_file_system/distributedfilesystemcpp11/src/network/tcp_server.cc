#include "network/tcp_server.hpp"
#include "common/logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept> 
static void set_reuse_addr(int fd) {
  int flag = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

dfs::TcpServer::TcpServer(const std::string& host, int port)
  : host_(host), port_(port), listen_fd_(-1) {}

void dfs::TcpServer::serve(OnConnect cb) {
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ < 0) throw std::runtime_error("socket");
  set_reuse_addr(listen_fd_);

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    throw std::runtime_error("bind");
  if (::listen(listen_fd_, 128) < 0)
    throw std::runtime_error("listen");

 // LOG_INFO("tcp listen %s:%d", host_.c_str(), port_);

  while (true) {
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    int fd = accept(listen_fd_, (struct sockaddr*)&cli, &len);
   // if (fd < 0) { LOG_ERROR("accept: %s", strerror(errno)); continue; }
    std::string cli_addr = inet_ntoa(cli.sin_addr);
    cb(fd, cli_addr);
  }
}
