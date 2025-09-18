#ifndef TINY_COR_HPP
#define TINY_COR_HPP
/*
 * tiny-cor —— Ubuntu 22.04 / g++-11 零警告零错误版
 * g++ -std=c++11 -O2 -DTCOR_MAIN tiny_cor.hpp -pthread -o tcor
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdexcept>
#include <vector>
#include <functional>
#include <map>
#include <thread>
#include <cstdio>
#include <queue>
#include <signal.h>

/* ------------------------------------------------------------------ */
/*  常量定义                                                           */
/* ------------------------------------------------------------------ */
#define MAX_EVENTS 64
#define MAX_COR 100000
#define STACK_SIZE (64 * 1024)

/* ------------------------------------------------------------------ */
/*  上下文切换                                                         */
/* ------------------------------------------------------------------ */
struct context { 
    void* rsp; 
    void* rbp;
    void* rbx;
    void* r12;
    void* r13;
    void* r14;
    void* r15;
};

#if defined(__x86_64__)
static inline void ctx_switch(context* old_, context* new_) {
    asm volatile(
        "pushq %%rbp\n\t"
        "pushq %%rbx\n\t"
        "pushq %%r12\n\t"
        "pushq %%r13\n\t"
        "pushq %%r14\n\t"
        "pushq %%r15\n\t"
        "movq %%rsp, %0\n\t"
        "movq %%rbp, %1\n\t"
        "movq %%rbx, %2\n\t"
        "movq %%r12, %3\n\t"
        "movq %%r13, %4\n\t"
        "movq %%r14, %5\n\t"
        "movq %%r15, %6\n\t"
        "movq %7, %%rsp\n\t"
        "movq %8, %%rbp\n\t"
        "movq %9, %%rbx\n\t"
        "movq %10, %%r12\n\t"
        "movq %11, %%r13\n\t"
        "movq %12, %%r14\n\t"
        "movq %13, %%r15\n\t"
        "popq %%r15\n\t"
        "popq %%r14\n\t"
        "popq %%r13\n\t"
        "popq %%r12\n\t"
        "popq %%rbx\n\t"
        "popq %%rbp\n\t"
        "ret"
        : "=m"(old_->rsp), "=m"(old_->rbp), "=m"(old_->rbx),
          "=m"(old_->r12), "=m"(old_->r13), "=m"(old_->r14), "=m"(old_->r15)
        : "m"(new_->rsp), "m"(new_->rbp), "m"(new_->rbx),
          "m"(new_->r12), "m"(new_->r13), "m"(new_->r14), "m"(new_->r15)
        : "memory");
}
#elif defined(__aarch64__)
static inline void ctx_switch(context* old_, context* new_) {
    asm volatile(
        "stp x19, x20, [sp, #-16]!\n\t"
        "stp x21, x22, [sp, #-16]!\n\t"
        "stp x23, x24, [sp, #-16]!\n\t"
        "stp x25, x26, [sp, #-16]!\n\t"
        "stp x27, x28, [sp, #-16]!\n\t"
        "stp x29, x30, [sp, #-16]!\n\t"
        "mov x8, sp\n\t"
        "str x8, [%0]\n\t"
        "ldr x8, [%1]\n\t"
        "mov sp, x8\n\t"
        "ldp x29, x30, [sp], #16\n\t"
        "ldp x27, x28, [sp], #16\n\t"
        "ldp x25, x26, [sp], #16\n\t"
        "ldp x23, x24, [sp], #16\n\t"
        "ldp x21, x22, [sp], #16\n\t"
        "ldp x19, x20, [sp], #16\n\t"
        "ret"
        :
        : "r"(old_), "r"(new_)
        : "memory");
}
#endif

namespace tcr {

/* ------------------------------------------------------------------ */
/*  简单的线程安全队列                                                 */
/* ------------------------------------------------------------------ */
template <typename T>
class safe_queue {
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
public:
    void push(T v) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(v));
        cv_.notify_one();
    }
    
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

/* ------------------------------------------------------------------ */
/*  调度器前向声明                                                     */
/* ------------------------------------------------------------------ */
class scheduler;

/* ------------------------------------------------------------------ */
/*  协程对象                                                           */
/* ------------------------------------------------------------------ */
class coroutine {
    friend class scheduler;
public:
    using fn_t = void (*)(void*);
    enum status { READY, RUN, WAIT_IO, DEAD };
private:
    fn_t            entry_;
    void*           arg_;
    context         ctx_;
    status          st_;
    coroutine*      next_;
    void*           stack_;
    size_t          stack_size_;
    scheduler*      sched_;

    static void trampoline(void*);
public:
    coroutine(fn_t f, void* arg, scheduler* s);
    ~coroutine();
    void resume();
    void yield();
    status state() const { return st_; }
};

/* ------------------------------------------------------------------ */
/*  调度器                                                             */
/* ------------------------------------------------------------------ */
class scheduler {
    friend class coroutine;
    int                id_;
    pthread_t          tid_;
    context            sched_ctx_;
    coroutine*         current_;
    safe_queue<coroutine*> runq_;
    std::mutex         sleep_mu_;
    std::condition_variable sleep_cv_;
    int                ep_fd_{-1};
    
    void loop();
    static void* thread_fn(void* p) {
        ((scheduler*)p)->loop();
        return nullptr;
    }
    
    coroutine* steal();
public:
    explicit scheduler(int id);
    ~scheduler();
    void spawn(coroutine::fn_t f, void* arg = nullptr);
    void schedule(coroutine* co);
    void wait_fd(int fd, int events);
    coroutine* current() { return current_; }
    static scheduler* local();
};

/* ------------------------------------------------------------------ */
/*  全局数组                                                           */
/* ------------------------------------------------------------------ */
static scheduler* g_scheds[64];
static int        g_ncpu = 0;

/* ------------------------------------------------------------------ */
/*  协程实现                                                           */
/* ------------------------------------------------------------------ */
coroutine::coroutine(fn_t f, void* arg, scheduler* s)
    : entry_(f), arg_(arg), st_(READY), next_(nullptr),
      stack_size_(STACK_SIZE), sched_(s) {
    stack_ = mmap(nullptr, stack_size_, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack_ == MAP_FAILED) throw std::bad_alloc();
    
    // 设置栈顶指针
    char* top = (char*)stack_ + stack_size_;
    
    // 对齐到16字节边界
    top = (char*)((uintptr_t)top & ~0xF);
    
    // 保存返回地址
    top -= sizeof(void*);
    *(void**)top = (void*)trampoline;
    
    // 设置上下文
    ctx_.rsp = top;
}

coroutine::~coroutine() {
    if (stack_) {
        munmap(stack_, stack_size_);
    }
}

void coroutine::trampoline(void* arg) {
    coroutine* co = (coroutine*)arg;
    co->entry_(co->arg_);
    co->st_ = DEAD;
    co->yield();
}

void coroutine::resume() {
    sched_->current_ = this;
    st_ = RUN;
    ctx_switch(&sched_->sched_ctx_, &ctx_);
}

void coroutine::yield() {
    sched_->schedule(this);
    ctx_switch(&ctx_, &sched_->sched_ctx_);
}

/* ------------------------------------------------------------------ */
/*  调度器实现                                                         */
/* ------------------------------------------------------------------ */
scheduler::scheduler(int id) : id_(id), current_(nullptr) {
    ep_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (ep_fd_ < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }
    
    pthread_create(&tid_, nullptr, thread_fn, this);
    
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(id, &set);
    pthread_setaffinity_np(tid_, sizeof(set), &set);
    
    g_scheds[id] = this;
}

scheduler::~scheduler() {
    if (ep_fd_ >= 0) {
        close(ep_fd_);
    }
}

void scheduler::spawn(coroutine::fn_t f, void* arg) {
    coroutine* co = new coroutine(f, arg, this);
    schedule(co);
}

void scheduler::schedule(coroutine* co) {
    co->st_ = coroutine::READY;
    runq_.push(co);
}

scheduler* scheduler::local() {
    int cpu = sched_getcpu();
    if (cpu < 0 || cpu >= g_ncpu) cpu = 0;
    return g_scheds[cpu];
}

void scheduler::wait_fd(int fd, int events) {
    coroutine* co = current_;
    co->st_ = coroutine::WAIT_IO;
    
    struct epoll_event ev;
    ev.events = events | EPOLLET; // 边缘触发模式
    ev.data.ptr = co;
    
    if (epoll_ctl(ep_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        if (errno == EEXIST) {
            epoll_ctl(ep_fd_, EPOLL_CTL_MOD, fd, &ev);
        }
    }
    
    co->yield();
}

coroutine* scheduler::steal() {
    for (int i = 0; i < g_ncpu; ++i) {
        if (i == id_) continue;
        
        scheduler* s = g_scheds[i];
        if (!s) continue;
        
        coroutine* co = nullptr;
        if (s->runq_.pop(co)) {
            return co;
        }
    }
    return nullptr;
}

void scheduler::loop() {
    struct epoll_event events[MAX_EVENTS];
    const int max_events = MAX_EVENTS;
    
    while (true) {
        coroutine* co = nullptr;
        
        // 首先尝试从本地队列获取
        if (runq_.pop(co)) {
            co->resume();
            continue;
        }
        
        // 尝试从其他队列窃取
        co = steal();
        if (co) {
            co->resume();
            continue;
        }
        
        // 等待事件
        int n = epoll_wait(ep_fd_, events, max_events, 10); // 10ms超时
        for (int i = 0; i < n; i++) {
            coroutine* wco = static_cast<coroutine*>(events[i].data.ptr);
            wco->st_ = coroutine::READY;
            runq_.push(wco);
        }
        
        // 短暂休眠避免忙等待
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

/* ------------------------------------------------------------------ */
/*  定时任务调度器                                                     */
/* ------------------------------------------------------------------ */
class timer_scheduler {
    std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> timers;
    std::mutex mutex;
    std::condition_variable cv;
    bool running = true;
    
    void worker() {
        while (running) {
            std::unique_lock<std::mutex> lock(mutex);
            if (timers.empty()) {
                cv.wait(lock);
                continue;
            }
            
            auto next = timers.begin();
            auto now = std::chrono::steady_clock::now();
            
            if (next->first <= now) {
                auto task = next->second;
                timers.erase(next);
                lock.unlock();
                
                try {
                    task();
                } catch (...) {
                    // 忽略任务执行中的异常
                }
            } else {
                cv.wait_until(lock, next->first);
            }
        }
    }
    
public:
    timer_scheduler() {
        std::thread([this] { worker(); }).detach();
    }
    
    ~timer_scheduler() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
        }
        cv.notify_all();
    }
    
    void schedule(std::chrono::milliseconds delay, std::function<void()> task) {
        auto time = std::chrono::steady_clock::now() + delay;
        std::lock_guard<std::mutex> lock(mutex);
        timers.emplace(time, std::move(task));
        cv.notify_one();
    }
};

/* ------------------------------------------------------------------ */
/*  全局初始化                                                         */
/* ------------------------------------------------------------------ */
static void init_schedulers() {
    g_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (g_ncpu > 64) g_ncpu = 64;
    if (g_ncpu <= 0) g_ncpu = 1;
    
    for (int i = 0; i < g_ncpu; ++i) {
        g_scheds[i] = new scheduler(i);
    }
}

/* ------------------------------------------------------------------ */
/*  网络辅助函数                                                       */
/* ------------------------------------------------------------------ */
static int set_nonblock(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static int tcp_listen(int port) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) return -1;
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    if (listen(fd, 128) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

/* ------------------------------------------------------------------ */
/*  示例：echo server                                                  */
/* ------------------------------------------------------------------ */
static void echo_client(void* arg) {
    int fd = (int)(intptr_t)arg;
    char buf[1024];
    
    while (1) {
        tcr::scheduler::local()->wait_fd(fd, EPOLLIN);
        
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        
        tcr::scheduler::local()->wait_fd(fd, EPOLLOUT);
        write(fd, buf, n);
    }
    
    close(fd);
}

static void echo_server(void*) {
    int listen_fd = tcp_listen(8000);
    if (listen_fd < 0) return;
    
    printf("Echo server listening on port 8000\n");
    
    while (1) {
        tcr::scheduler::local()->wait_fd(listen_fd, EPOLLIN);
        
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int cli = accept4(listen_fd, (sockaddr*)&client_addr, &len, SOCK_NONBLOCK);
        if (cli < 0) continue;
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("New connection from %s:%d\n", ip, ntohs(client_addr.sin_port));
        
        tcr::scheduler::local()->spawn(echo_client, (void*)(intptr_t)cli);
    }
}

} // namespace tcr

/* ------------------------------------------------------------------ */
/*  信号处理                                                           */
/* ------------------------------------------------------------------ */
static void signal_handler(int sig) {
    printf("\nReceived signal %d, cleaning up...\n", sig);
    exit(0);
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */
#define TCOR_MAIN 1
#ifdef TCOR_MAIN
int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        printf("Starting tiny-cor on 2-core 2GB system...\n");
        tcr::init_schedulers();
        
        // 创建并启动echo服务器
        for (int i = 0; i < tcr::g_ncpu; i++) {
            tcr::scheduler* sched = tcr::g_scheds[i];
            if (sched) {
                sched->spawn(tcr::echo_server, nullptr);
            }
        }
        
        printf("Server running. Press Ctrl+C to stop.\n");
        
        // 添加一个定时任务示例
        static tcr::timer_scheduler timer;
        timer.schedule(std::chrono::seconds(5), [] {
            printf("System status: %d schedulers running\n", tcr::g_ncpu);
        });
        
        // 主线程保持运行
        pause();
        
    } catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
#endif
#endif // TINY_COR_HPP