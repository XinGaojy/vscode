#include <ucontext.h>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <sys/eventfd.h>

// ==================== 协程核心 ====================
class Coroutine {
public:
    using Func = std::function<void()>;
    
    explicit Coroutine(Func&& f, size_t stack_size = 64 * 1024) 
        : _func(std::move(f)) {
        _stack = new char[stack_size];
        getcontext(&_ctx);
        _ctx.uc_stack.ss_sp = _stack;
        _ctx.uc_stack.ss_size = stack_size;
        _ctx.uc_link = nullptr;
        
        makecontext(&_ctx, []() {
            auto self = current();
            self->_func();
            self->_finished = true;
            self->yield();
        }, 0);
    }
    
    ~Coroutine() {
        delete[] _stack;
    }
    
    void resume() {
        _caller = current();
        current() = this;
        swapcontext(&_caller->_ctx, &_ctx);
    }
    
    static void yield() {
        auto self = current();
        current() = self->_caller;
        swapcontext(&self->_ctx, &self->_caller->_ctx);
    }
    
    bool finished() const { return _finished; }
    
    static thread_local Coroutine*& current() {
        static thread_local Coroutine* _current = nullptr;
        return _current;
    }

private:
    ucontext_t _ctx;
    char* _stack;
    Func _func;
    bool _finished = false;
    Coroutine* _caller = nullptr;
};

// ==================== IO引擎 ====================
class IOEngine {
public:
    IOEngine() {
        _epoll_fd = epoll_create1(0);
        _event_fd = eventfd(0, EFD_NONBLOCK);
        
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = _event_fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _event_fd, &ev);
    }
    
    ~IOEngine() {
        close(_epoll_fd);
        close(_event_fd);
    }
    
    bool add_fd(int fd, Coroutine* co) {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = co;
        
        std::lock_guard lock(_mutex);
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            return false;
        }
        _pending_io[fd] = co;
        return true;
    }
    
    void poll(int timeout_ms) {
        constexpr int max_events = 64;
        epoll_event events[max_events];
        
        int n = epoll_wait(_epoll_fd, events, max_events, timeout_ms);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == _event_fd) {
                uint64_t val;
                (void)read(_event_fd, &val, sizeof(val));
                continue;
            }
            
            auto* co = static_cast<Coroutine*>(events[i].data.ptr);
            co->resume();
            
            std::lock_guard lock(_mutex);
            _pending_io.erase(events[i].data.fd);
        }
    }
    
    void wakeup() {
        uint64_t val = 1;
        (void)write(_event_fd, &val, sizeof(val));
    }

private:
    int _epoll_fd;
    int _event_fd;
    std::unordered_map<int, Coroutine*> _pending_io;
    std::mutex _mutex;
};

// ==================== 调度器 ====================
class Scheduler {
public:
    static Scheduler& instance() {
        static Scheduler sched;
        return sched;
    }
    
    void spawn(Coroutine::Func f) {
        std::lock_guard lock(_queue_mutex);
        _ready_queue.push(std::make_unique<Coroutine>(std::move(f)));
        _io_engine.wakeup();
    }
    
    void run() {
        _running = true;
        for (int i = 0; i < static_cast<int>(std::thread::hardware_concurrency()); ++i) {
            _workers.emplace_back([this] { worker_loop(); });
        }
        
        for (auto& t : _workers) {
            if (t.joinable()) t.join();
        }
    }
    
    void stop() {
        _running = false;
        _io_engine.wakeup();
    }

private:
    Scheduler() = default;
    
    void worker_loop() {
        while (_running) {
            std::unique_ptr<Coroutine> co;
            {
                std::lock_guard lock(_queue_mutex);
                if (!_ready_queue.empty()) {
                    co = std::move(_ready_queue.front());
                    _ready_queue.pop();
                }
            }
            
            if (co) {
                co->resume();
                if (!co->finished()) {
                    std::lock_guard lock(_queue_mutex);
                    _ready_queue.push(std::move(co));
                }
            } else {
                _io_engine.poll(10); // 10ms超时
            }
        }
    }
    
    std::vector<std::thread> _workers;
    std::queue<std::unique_ptr<Coroutine>> _ready_queue;
    std::mutex _queue_mutex;
    IOEngine _io_engine;
    std::atomic<bool> _running{false};
};

// ==================== 示例应用 ====================
void io_task() {
    int fd = open("test.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        std::cout << "Read " << n << " bytes: " 
                  << std::string_view(buf, n) << std::endl;
    }
    close(fd);
}

void cpu_task(int id) {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Task " << id << " step " << i << std::endl;
        Coroutine::yield();
    }
}

int main() {
    // 准备测试文件
    int fd = open("test.txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
    write(fd, "Hello Flare Coroutine", 21);
    close(fd);

    // 启动IO任务
    Scheduler::instance().spawn(io_task);

    // 启动CPU任务
    for (int i = 0; i < 5; ++i) {
        Scheduler::instance().spawn([i] { cpu_task(i); });
    }

    // 运行调度器
    Scheduler::instance().run();
    return 0;
}
