#ifndef TINY_COR_HPP
#define TINY_COR_HPP
/*
 * tiny-cor —— Ubuntu 22.04 2核2G安全版
 * g++ -std=c++11 -O2 -DTCOR_MAIN tiny_cor_fixed.hpp -pthread -o tcor_fixed
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
#include <signal.h>
#include <queue>
#include <signal.h>

/* ------------------------------------------------------------------ */
/*  常量定义 - 针对2核2G优化                                           */
/* ------------------------------------------------------------------ */
#define MAX_EVENTS 16         // 减少epoll事件数量
#define MAX_COR 500          // 减少最大协程数量
#define STACK_SIZE (64 * 1024) // 增加栈大小到64KB确保安全
#define MAX_SCHEDULERS 2     // 限制调度器数量为2个
#define BUF_SIZE 1024        // 增加缓冲区大小
#define LISTEN_BACKLOG 32    // 减少backlog

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
    void*           stack_;
    size_t          stack_size_;
    scheduler*      sched_;
    std::atomic<bool> valid_{true};

    static void trampoline(void*);
public:
    coroutine(fn_t f, void* arg, scheduler* s);
    ~coroutine();
    void resume();
    void yield();
    status state() const { return st_; }
    bool is_valid() const { return valid_.load(); }
    static void yield_current();
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
    int                ep_fd_{-1};
    std::atomic<bool>  running_{true};
    std::atomic<int>   coroutine_count_{0};
    
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
    void stop() { running_ = false; }
    int get_coroutine_count() const { return coroutine_count_.load(); }
};

/* ------------------------------------------------------------------ */
/*  定时任务调度器                                                     */
/* ------------------------------------------------------------------ */
class timer_scheduler {
    std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> timers;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    std::thread worker_thread;
    
    void worker() {
        while (running.load()) {
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
        worker_thread = std::thread([this] { worker(); });
    }
    
    ~timer_scheduler() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
        }
        cv.notify_all();
        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }
    
    void schedule(std::chrono::milliseconds delay, std::function<void()> task) {
        auto time = std::chrono::steady_clock::now() + delay;
        std::lock_guard<std::mutex> lock(mutex);
        timers.emplace(time, std::move(task));
        cv.notify_one();
    }
};

/* ------------------------------------------------------------------ */
/*  全局数组                                                           */
/* ------------------------------------------------------------------ */
static scheduler* g_scheds[MAX_SCHEDULERS];
static int        g_ncpu = 0;
static std::atomic<bool> g_initialized{false};
static std::atomic<int> g_total_coroutines{0};

/* ------------------------------------------------------------------ */
/*  协程实现                                                           */
/* ------------------------------------------------------------------ */
coroutine::coroutine(fn_t f, void* arg, scheduler* s)
    : entry_(f), arg_(arg), st_(READY), stack_size_(STACK_SIZE), sched_(s) {
    // 检查协程数量限制
    if (g_total_coroutines.load() >= MAX_COR) {
        throw std::runtime_error("Too many coroutines");
    }
    
    // 分配栈内存
    stack_ = mmap(nullptr, stack_size_, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack_ == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate stack memory");
    }
    
    // 设置栈顶指针 - 修复栈对齐问题
    char* top = (char*)stack_ + stack_size_;
    
    // 对齐到16字节边界
    top = (char*)((uintptr_t)top & ~0xF);
    
    // 为trampoline函数预留参数空间
    top -= sizeof(void*);
    *(void**)top = this;  // 传递this指针给trampoline
    
    // 保存返回地址
    top -= sizeof(void*);
    *(void**)top = (void*)trampoline;
    
    // 设置上下文
    ctx_.rsp = top;
    
    // 初始化其他寄存器为0
    ctx_.rbp = nullptr;
    ctx_.rbx = nullptr;
    ctx_.r12 = nullptr;
    ctx_.r13 = nullptr;
    ctx_.r14 = nullptr;
    ctx_.r15 = nullptr;
    
    // 增加协程计数
    g_total_coroutines.fetch_add(1);
    sched_->coroutine_count_.fetch_add(1);
    
        printf("hhhhhhhhh--------");
        
        printf("hhhhhhhhh--------");
}

coroutine::~coroutine() {
    if (stack_) {
        munmap(stack_, stack_size_);
    }
    valid_.store(false);
    
    // 减少协程计数
    g_total_coroutines.fetch_sub(1);
    if (sched_) {
        sched_->coroutine_count_.fetch_sub(1);
    }
}

void coroutine::trampoline(void* arg) {
    coroutine* co = (coroutine*)arg;
    if (!co || !co->valid_.load()) {
        return;
    }
    
    try {
        co->entry_(co->arg_);
    } catch (const std::exception& e) {
        fprintf(stderr, "Coroutine exception: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "Coroutine unknown exception\n");
    }
    
    co->st_ = DEAD;
    co->yield();
}

void coroutine::resume() {
    if (st_ == DEAD || !valid_.load()) return;
    
    scheduler* old_sched = sched_;
    if (!old_sched) return;
    
    old_sched->current_ = this;
    st_ = RUN;
    ctx_switch(&old_sched->sched_ctx_, &ctx_);
}

void coroutine::yield() {
    if (st_ == DEAD || !valid_.load()) return;
    
    scheduler* old_sched = sched_;
    if (!old_sched) return;
    
    old_sched->schedule(this);
    ctx_switch(&ctx_, &old_sched->sched_ctx_);
}

void coroutine::yield_current() {
    scheduler* sched = scheduler::local();
    if (sched && sched->current_ && sched->current_->valid_.load()) {
        sched->current_->yield();
    }
}

/* ------------------------------------------------------------------ */
/*  调度器实现                                                         */
/* ------------------------------------------------------------------ */
scheduler::scheduler(int id) : id_(id), current_(nullptr) {
    ep_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (ep_fd_ < 0) {
        throw std::runtime_error("epoll_create1 failed");
    }
    
    // 初始化调度器上下文
    memset(&sched_ctx_, 0, sizeof(sched_ctx_));
    
    int ret = pthread_create(&tid_, nullptr, thread_fn, this);
    if (ret != 0) {
        close(ep_fd_);
        throw std::runtime_error("pthread_create failed");
    }
    
    // 设置CPU亲和性
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(id % g_ncpu, &set);
    pthread_setaffinity_np(tid_, sizeof(set), &set);
    
    g_scheds[id] = this;
}

scheduler::~scheduler() {
    stop();
    if (tid_) {
        pthread_join(tid_, nullptr);
    }
    if (ep_fd_ >= 0) {
        close(ep_fd_);
    }
}

void scheduler::spawn(coroutine::fn_t f, void* arg) {
    printf("----------");
    try {
        printf("hh----------");
        coroutine* co = new coroutine(f, arg, this);
        printf("hhhhhhhhh--------");
        schedule(co);
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to spawn coroutine: %s\n", e.what());
    }
}

void scheduler::schedule(coroutine* co) {
    if (co && co->st_ != coroutine::DEAD && co->valid_.load()) {
        co->st_ = coroutine::READY;
        
        printf("ffffffffff--------");
        runq_.push(co);
        
        printf("fffffffffff--------");
    }
}

scheduler* scheduler::local() {
    if (!g_initialized.load()) {
        return nullptr;
    }
    
    int cpu = sched_getcpu();
    if (cpu < 0 || cpu >= g_ncpu) cpu = 0;
    return g_scheds[cpu];
}

void scheduler::wait_fd(int fd, int events) {
    if (!current_ || !current_->valid_.load()) return;
    
    coroutine* co = current_;
    co->st_ = coroutine::WAIT_IO;
    
    struct epoll_event ev;
    ev.events = events | EPOLLET; // 边缘触发模式
    ev.data.ptr = co;
    
    int ret = epoll_ctl(ep_fd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        if (errno == EEXIST) {
            epoll_ctl(ep_fd_, EPOLL_CTL_MOD, fd, &ev);
        } else {
            // 如果epoll操作失败，直接恢复协程状态
            co->st_ = coroutine::READY;
            return;
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
        if (s->runq_.try_pop(co)) {
            return co;
        }
    }
    return nullptr;
}

void scheduler::loop() {
    struct epoll_event events[MAX_EVENTS];
    const int max_events = MAX_EVENTS;
    
    while (running_.load()) {
        coroutine* co = nullptr;
        
        // 首先尝试从本地队列获取
        if (runq_.pop(co)) {
            if (co && co->st_ != coroutine::DEAD && co->valid_.load()) {
                co->resume();
            }
            continue;
        }
        
        // 尝试从其他队列窃取
        co = steal();
        if (co && co->st_ != coroutine::DEAD && co->valid_.load()) {
            co->resume();
            continue;
        }
        
        printf("bagabgabgabgabgabgab");
        // 等待事件
        int n = epoll_wait(ep_fd_, events, max_events, 50); // 增加超时时间到50ms
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                coroutine* wco = static_cast<coroutine*>(events[i].data.ptr);
                if (wco && wco->st_ == coroutine::WAIT_IO && wco->valid_.load()) {
                    wco->st_ = coroutine::READY;
                    runq_.push(wco);
                }
            }
        } else if (n < 0 && errno != EINTR) {
            // epoll错误，短暂休眠
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // 短暂休眠避免忙等待
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

/* ------------------------------------------------------------------ */
/*  全局初始化                                                         */
/* ------------------------------------------------------------------ */
static void           init_schedulers() {
    if (g_initialized.exchange(true)) {
        return; // 已经初始化过了
    }
    
    g_ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (g_ncpu > MAX_SCHEDULERS) g_ncpu = MAX_SCHEDULERS;
    if (g_ncpu <= 0) g_ncpu = 1;
    
    printf("Initializing %d schedulers for %d CPU cores\n", g_ncpu, g_ncpu);
    
    // 初始化全局数组
    for (int i = 0; i < MAX_SCHEDULERS; ++i) {
        g_scheds[i] = nullptr;
    }
    
    for (int i = 0; i < g_ncpu; ++i) {
        try {
            g_scheds[i] = new scheduler(i);
            printf("Scheduler %d initialized\n", i);
        } catch (const std::exception& e) {
            fprintf(stderr, "Failed to create scheduler %d: %s\n", i, e.what());
            // 如果某个调度器创建失败，清理已创建的调度器
            for (int j = 0; j < i; ++j) {
                if (g_scheds[j]) {
                    delete g_scheds[j];
                    g_scheds[j] = nullptr;
                }
            }
            g_initialized = false;
            throw;
        }
    }
}

static void cleanup_schedulers() {
    printf("Cleaning up schedulers...\n");
    for (int i = 0; i < g_ncpu; ++i) {
        if (g_scheds[i]) {
            delete g_scheds[i];
            g_scheds[i] = nullptr;
        }
    }
    g_initialized = false;
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
    
    if (listen(fd, LISTEN_BACKLOG) < 0) {
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
    char buf[BUF_SIZE];
    
    while (1) {
        scheduler* sched = scheduler::local();
        if (!sched) break;
        
        sched->wait_fd(fd, EPOLLIN);
        
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        
        sched->wait_fd(fd, EPOLLOUT);
        ssize_t written = write(fd, buf, n);
        if (written < 0) {
            break;
        }
    }
    
    close(fd);
}

static void echo_server(void*) {
    int listen_fd = tcp_listen(8000);
    if (listen_fd < 0) {
        printf("Failed to create listen socket\n");
        return;
    }
    
    printf("Echo server listening on port 8000\n");
    
    while (1) {
        scheduler* sched = scheduler::local();
        if (!sched) break;
        
        sched->wait_fd(listen_fd, EPOLLIN);
        
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int cli = accept4(listen_fd, (sockaddr*)&client_addr, &addr_len, SOCK_NONBLOCK);
        if (cli < 0) continue;
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("New connection from %s:%d\n", ip, ntohs(client_addr.sin_port));
        
        sched->spawn(echo_client, (void*)(intptr_t)cli);
    }
}

} // namespace tcr

/* ------------------------------------------------------------------ */
/*  信号处理                                                           */
/* ------------------------------------------------------------------ */
static void signal_handler(int sig) {
    printf("\nReceived signal %d, cleaning up...\n", sig);
    tcr::cleanup_schedulers();
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
        // 输出启动信息
        printf("Starting tiny-cor on 2-core 2GB system...\n");
        // 初始化调度器
        tcr::init_schedulers();
        
        // 获取本地调度器
        auto sched = tcr::scheduler::local();
        if (!sched) {
            // 获取本地调度器失败
            printf("Failed to get local scheduler\n");
            return 1;
        }
        
        // 输出启动回声服务器信息
        printf("Starting echo server...\n");
        // 启动回声服务器
        sched->spawn(tcr::echo_server, nullptr);
        
        printf("hhhhhhhhh--------");
        // 添加一个定时任务示例
        // 输出一个分隔符
        printf("hhhhhhhhh--------");
        // 添加一个定时任务示例
        static tcr::timer_scheduler timer;
        timer.schedule(std::chrono::seconds(5), [] {
            // 输出系统状态信息
            printf("System status: %d total coroutines\n", 
                   tcr::g_total_coroutines.load());
        });
        // 输出服务器运行状态信息
        printf("Server running. Press Ctrl+C to stop.\n");
        // 暂停程序，等待信号
        pause();
    } catch (const std::exception& e) {
        // 捕获异常并输出错误信息
        printf("Error: %s\n", e.what());
        return 1;
    }
    // 清理调度器
    tcr::cleanup_schedulers();
    return 0;
}
#endif
#endif // TINY_COR_HPP