#include "common/logger.hpp"
#include <cstdio>
#include <cstdarg>
#include <ctime>

static const char* level_str(const char* level) {
    static char buf[32];
    time_t now = time(nullptr);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

void dfs::init_logger() {
    // 简单控制台日志，生产可换 spdlog
}

void dfs::log_info(const char* fmt, ...) {
    char tmp[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    std::printf("[%s] [info] %s\n", level_str("INFO"), tmp);
    fflush(stdout);
}

void dfs::log_error(const char* fmt, ...) {
    char tmp[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    std::fprintf(stderr, "[%s] [error] %s\n", level_str("ERROR"), tmp);
    fflush(stderr);
}
