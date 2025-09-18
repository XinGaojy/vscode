#pragma once
#include <liburing.h>
#include <vector>
#include <queue>
#include <atomic>

class IoUringScheduler {
public:
    explicit IoUringScheduler(size_t entries = 4096);
    ~IoUringScheduler();

    // 提交异步读操作
    class ReadAwaiter : public IoUringAwaiter {
    public:
        ReadAwaiter(int fd, void* buf, size_t len, off_t offset)
            : m_fd(fd), m_buf(buf), m_len(len), m_offset(offset) {}
        void submit_io() override;
    private:
        int m_fd;
        void* m_buf;
        size_t m_len;
        off_t m_offset;
    };

    // 运行事件循环
    void run();
    // 停止事件循环
    void stop();

private:
    io_uring m_ring;
    std::atomic<bool> m_running{false};
    std::queue<std::coroutine_handle<>> m_ready_coros;
};