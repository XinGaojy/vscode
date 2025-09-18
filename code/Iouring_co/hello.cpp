#include <coroutine>
#include <liburing.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include<thread>
#include<cstring>
// -------------------- 协程定义 --------------------
struct Task {
    struct promise_type {
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
        Task get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
    };
    std::coroutine_handle<promise_type> handle;
};

// -------------------- io_uring调度器 --------------------
class IoUringScheduler {
public:
    explicit IoUringScheduler(size_t entries = 8) {  // 使用更小的队列
        // 尝试多种初始化方式
        if (io_uring_queue_init(entries, &ring_, 0) < 0) {
            if (entries > 2) {
                // 递归减小队列大小
                throw std::runtime_error(
                    "io_uring init failed at size " + std::to_string(entries) + 
                    ": " + strerror(errno)
                );
            } else {
                throw std::runtime_error(
                    "io_uring not supported on this kernel: " + 
                    std::string(strerror(errno))
                );
            }
        }
    }

    class ReadAwaiter {
    public:
        ReadAwaiter(IoUringScheduler* sched, int fd, void* buf, size_t len)
            : sched_(sched), fd_(fd), buf_(buf), len_(len) {}

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            coro_ = h;
            io_uring_sqe* sqe = io_uring_get_sqe(&sched_->ring_);
            if (!sqe) {
                throw std::runtime_error("io_uring sqe get failed");
            }
            io_uring_prep_read(sqe, fd_, buf_, len_, 0);
            io_uring_sqe_set_data(sqe, this);
            if (io_uring_submit(&sched_->ring_) < 0) {
                throw std::runtime_error("io_uring submit failed");
            }
        }

        int await_resume() const noexcept { return result_; }

        int result_ = 0;
        std::coroutine_handle<> coro_;
    private:
        IoUringScheduler* sched_;
        int fd_;
        void* buf_;
        size_t len_;
    };

    void run() {
        while (true) {
            io_uring_cqe* cqe;
            int ret = io_uring_wait_cqe(&ring_, &cqe);
            if (ret < 0) {
                if (ret == -EINTR) continue;  // 处理信号中断
                throw std::runtime_error("io_uring wait error");
            }

            auto* awaiter = static_cast<ReadAwaiter*>(io_uring_cqe_get_data(cqe));
            awaiter->result_ = cqe->res;
            io_uring_cqe_seen(&ring_, cqe);
            awaiter->coro_.resume();
        }
    }

private:
    io_uring ring_;
};

// -------------------- 测试用例 --------------------
Task async_read(IoUringScheduler& sched, int fd, char* buf, size_t size) {
    auto result = co_await IoUringScheduler::ReadAwaiter(&sched, fd, buf, size);
    if (result < 0) {
        std::cerr << "Read failed: " << strerror(-result) << std::endl;
    } else {
        std::cout << "Read " << result << " bytes" << std::endl;
    }
}

int main() {
    try {
        // 创建测试文件
        int fd = open("test.txt", O_CREAT|O_RDWR|O_TRUNC, 0644);
        if (fd < 0) throw std::runtime_error("open failed");

        const char* text = "Hello, Tencent tlinux with io_uring!";
        write(fd, text, strlen(text));
        lseek(fd, 0, SEEK_SET);

        // 运行协程
        IoUringScheduler sched;
        char buf[256]{};
        async_read(sched, fd, buf, sizeof(buf));

        // 简单事件循环（实际生产环境需要更复杂的停止逻辑）
        sched.run();

        close(fd);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        
        // 备选方案提示
        std::cerr << "\n==== Fallback Suggestion ====\n"
                  << "1. Contact Tencent Cloud support to enable io_uring\n"
                  << "2. Use epoll instead:\n"
                  << "   #include <sys/epoll.h>\n"
                  << "3. Upgrade to newer kernel (e.g. tlinux 5.10+)\n";
        return 1;
    }
    return 0;
}