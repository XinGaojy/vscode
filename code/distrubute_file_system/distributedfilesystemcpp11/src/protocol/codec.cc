#include "protocol/codec.hpp"
#include <unistd.h>
#include <arpa/inet.h>

bool dfs::send_msg(int fd, const google::protobuf::Message& msg) {
  std::string buf = msg.SerializeAsString();
  uint32_t len = htonl(static_cast<uint32_t>(buf.size()));
  if (write(fd, &len, 4) != 4) return false;
  return write(fd, buf.data(), buf.size()) == static_cast<ssize_t>(buf.size());
}

bool dfs::recv_msg(int fd, google::protobuf::Message* msg) {
  uint32_t len_net;
  if (read(fd, &len_net, 4) != 4) return false;
  uint32_t len = ntohl(len_net);
  std::string buf(len, '\0');
  if (read(fd, &buf[0], len) != static_cast<ssize_t>(len)) return false;
  return msg->ParseFromString(buf);
}
