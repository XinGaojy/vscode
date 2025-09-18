#ifndef DFS_LOGGER_HPP
#define DFS_LOGGER_HPP

#include <string>

namespace dfs {

void init_logger();                 // 初始化终端日志
void log_info(const char* fmt, ...);
void log_error(const char* fmt, ...);

} // namespace dfs

#endif
