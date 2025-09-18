#ifndef DFS_CODEC_HPP
#define DFS_CODEC_HPP
#include <string>
#include <google/protobuf/message.h>

namespace dfs {

bool send_msg(int fd, const google::protobuf::Message& msg);
bool recv_msg(int fd, google::protobuf::Message* msg);

} // namespace dfs
#endif
