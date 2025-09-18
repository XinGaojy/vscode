#include <coroutine>
#include <vector>
#include <queue>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include <sys/eventfd.h>

// -------------------- 自动检测io_uring --------------------
#if __has_include(<liburing.h>)
#include <liburing.h>
#define HAVE_LIBURING 0
#endif

// -------------------- 协程任务定义 --------------------
struct Task {
    struct promise_type {
        void* scheduler = nullptr;  // 存储Worker指针
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
        Task get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
    };

    using handle_t = std::coroutine_handle<promise_type>;
    
    explicit Task(handle_t h) : handle_(h) {}
    ~Task() { if (handle_) handle_.destroy(); }

    void resume() { if (!handle_.done()) handle_.resume(); }
    handle_t get_handle() const { return handle_; }

private:
    handle_t handle_;
};

// -------------------- IO引擎抽象 --------------------
class IoEngine {
public:
    virtual ~IoEngine() = default;
    virtual bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) = 0;
    virtual void process_completions() = 0;
    virtual void stop() = 0;
};

// -------------------- io_uring引擎 --------------------
#ifdef HAVE_LIBURING
class IoUringEngine : public IoEngine {
public:
    IoUringEngine() {
        if (io_uring_queue_init(1024, &ring_, 0) < 0) {
            throw std::runtime_error("io_uring init failed");
        }
    }

    ~IoUringEngine() {
        io_uring_queue_exit(&ring_);
    }

    bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) override {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (!sqe) return false;

        io_uring_prep_read(sqe, fd, buf, len, 0);
        io_uring_sqe_set_data(sqe, h.address());
        return io_uring_submit(&ring_) == 1;
    }

    void process_completions() override {
        io_uring_cqe* cqe;
        unsigned head, count = 0;

        io_uring_for_each_cqe(&ring_, head, cqe) {
            ++count;
            auto h = std::coroutine_handle<>::from_address(io_uring_cqe_get_data(cqe));
            h.resume();
        }

        if (count > 0) io_uring_cq_advance(&ring_, count);
    }

    void stop() override {}

    io_uring& get_ring() { return ring_; }

private:
    io_uring ring_;
};
#endif

// -------------------- epoll引擎 --------------------
class EpollEngine : public IoEngine {
public:
    EpollEngine() {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ < 0) throw std::runtime_error("epoll create failed");

        event_fd_ = eventfd(0, EFD_NONBLOCK);
        if (event_fd_ < 0) {
            close(epoll_fd_);
            throw std::runtime_error("eventfd create failed");
        }

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = event_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd_, &ev) < 0) {
            close(epoll_fd_);
            close(event_fd_);
            throw std::runtime_error("epoll_ctl failed");
        }
    }

    ~EpollEngine() {
        close(epoll_fd_);
        close(event_fd_);
    }

    bool submit_read(int fd, void* buf, size_t len, std::coroutine_handle<> h) override {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = h.address();

        std::lock_guard lock(mutex_);
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
            return false;
        }

        pending_io_[fd] = h;
        return true;
    }

    void process_completions() override {
        constexpr int max_events = 64;
        epoll_event events[max_events];

        int n = epoll_wait(epoll_fd_, events, max_events, 0);
        for (int i = 0; i < n; ++i) {
            auto h = std::coroutine_handle<>::from_address(events[i].data.ptr);
            h.resume();

            std::lock_guard lock(mutex_);
            pending_io_.erase(events[i].data.fd);
        }
    }

    void stop() override {
        uint64_t val = 1;
        write(event_fd_, &val, sizeof(val));
    }

private:
    int epoll_fd_;
    int event_fd_;
    std::mutex mutex_;
    std::unordered_map<int, std::coroutine_handle<>> pending_io_;
};

// -------------------- 工作线程 --------------------
class Worker {
public:
    Worker(bool try_uring) {
        init_engine(try_uring);
    }

    ~Worker() {
        stop();
    }

    void init_engine(bool try_uring) {
// #ifdef HAVE_LIBURING
//         if (try_uring) {
//             try {
//                 engine_ = std::make_unique<IoUringEngine>();
//                 return;
//             } catch (...) {
//                 std::cerr << "Fallback to epoll" << std::endl;
//             }
//         }
// #endif
        engine_ = std::make_unique<EpollEngine>();
    }

    void run() {
        running_ = true;
        
        while (running_) {
            // 处理IO事件
            engine_->process_completions();

            // 执行本地任务
            std::queue<Task> local_queue;
            {
                std::lock_guard lock(queue_mutex_);
                local_queue.swap(ready_queue_);
            }

            while (!local_queue.empty() && running_) {
                auto task = std::move(local_queue.front());
                local_queue.pop();
                task.resume();
            }

            // 无任务时休眠
            if (local_queue.empty() && running_) {
#ifdef HAVE_LIBURING
                if (auto* uring = dynamic_cast<IoUringEngine*>(engine_.get())) {
                    io_uring_submit_and_wait(&uring->get_ring(), 1);
                } else 
#endif
                {
                    usleep(1000); // 1ms休眠
                }
            }
        }
    }

    void stop() {
        running_ = false;
        engine_->stop();
    }

    void enqueue(Task task) {
        std::lock_guard lock(queue_mutex_);
        ready_queue_.push(std::move(task));
    }

    IoEngine& engine() { return *engine_; }

    void* get_scheduler_ptr() { return this; }

private:
    std::unique_ptr<IoEngine> engine_;
    std::queue<Task> ready_queue_;
    std::mutex queue_mutex_;
    std::atomic<bool> running_{false};
};

// -------------------- 主调度器 --------------------
class Scheduler {
public:
    explicit Scheduler(size_t worker_count = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < worker_count; ++i) {
            workers_.emplace_back(std::make_unique<Worker>(true));
        }
    }

    ~Scheduler() {
        for (auto& w : workers_) w->stop();
    }

    void schedule(Task task) {
        // 绑定Worker指针到协程promise
        auto h = task.get_handle();
        h.promise().scheduler = workers_[next_worker_ % workers_.size()]->get_scheduler_ptr();
        
        workers_[next_worker_++ % workers_.size()]->enqueue(std::move(task));
    }

    void run() {
        std::vector<std::thread> threads;
        for (size_t i = 0; i < workers_.size(); ++i) {
            threads.emplace_back([this, i] {
                // 绑定CPU核心
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i % std::thread::hardware_concurrency(), &cpuset);
                pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

                workers_[i]->run();
            });
        }

        for (auto& t : threads) t.join();
    }

private:
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<size_t> next_worker_{0};
};

// -------------------- 示例应用 --------------------
Task async_read(Scheduler& sched, int fd, char* buf, size_t size) {
    struct ReadAwaitable {
        int fd;
        char* buf;
        size_t size;
        int result = -1;

        bool await_ready() const { return false; }

        void await_suspend(std::coroutine_handle<> h) {
            // 从通用句柄转换为Task特定句柄
            auto task_h = std::coroutine_handle<Task::promise_type>::from_address(h.address());
            void* scheduler_ptr = task_h.promise().scheduler;

            if (auto* worker = static_cast<Worker*>(scheduler_ptr)) {
                if (!worker->engine().submit_read(fd, buf, size, h)) {
                    // 降级为同步读取
                    result = read(fd, buf, size);
                    h.resume();
                }
            }
        }

        int await_resume() { return result; }
    };

    int ret = co_await ReadAwaitable{fd, buf, size};
    if (ret < 0) {
        std::cerr << "Read failed: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Read " << ret << " bytes: " 
                  << std::string_view(buf, ret) << std::endl;
    }
}

int main() {
    Scheduler sched;

    // 创建测试文件
    int fd = open("test.txt", O_CREAT|O_RDWR|O_TRUNC, 0644);
    const char* text = "Hello from fixed coroutine scheduler";
    write(fd, text, strlen(text));
    lseek(fd, 0, SEEK_SET);

    // 提交任务
    char buf[256]{};
    sched.schedule(async_read(sched, fd, buf, sizeof(buf)));

    // 运行调度器
    sched.run();
    close(fd);
    return 0;
}