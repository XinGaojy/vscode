#include "network/tcp_client.hpp"
#include "common/logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <netdb.h>

static void set_reuse_addr(int fd) {
    int flag = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

namespace dfs {

TcpClient::TcpClient(const std::string& host, int port)
    : host_(host), port_(port), fd_(-1) {}

bool TcpClient::connect() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        log_error("socket: %s", strerror(errno));
        return false;
    }
    set_reuse_addr(fd_);

    struct hostent* he = gethostbyname(host_.c_str());
    if (!he) {
        log_error("gethostbyname %s: %s", host_.c_str(), hstrerror(h_errno));
        close();
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_error("connect %s:%d: %s", host_.c_str(), port_, strerror(errno));
        close();
        return false;
    }
    log_info("tcp connected to %s:%d", host_.c_str(), port_);
    return true;
}

void TcpClient::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

TcpClient::~TcpClient() {
    close();
}

} // namespace dfs
