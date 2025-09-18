#ifndef DFS_UTIL_HPP
#define DFS_UTIL_HPP

#include <string>

namespace dfs { namespace util {

inline std::string format(const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return std::string(buf);
}

}} // namespace dfs::util

#endif
