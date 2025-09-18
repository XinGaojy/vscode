#include "iouring_coroutine.hpp"
#include "iouring_scheduler.hpp"
#include <fcntl.h>
#include <iostream>

// 异步读文件协程
Coroutine async_read_file(IoUringScheduler& sched, int fd, char* buf, size_t size) {
    auto result = co_await sched.ReadAwaiter(fd, buf, size, 0);
    if (result < 0) {
        std::cerr << "Read failed: " << result << std::endl;
    } else {
        std::cout << "Read " << result << " bytes: " << std::string_view(buf, result) << std::endl;
    }
}

int main() {
    IoUringScheduler sched;

    // 打开测试文件
    int fd = open("test.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char buf[4096]{};
    async_read_file(sched, fd, buf, sizeof(buf));

    // 运行事件循环
    sched.run();
    close(fd);
    return 0;
}