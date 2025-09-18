#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cerrno>
#include <cstdint>

// 协程上下文结构
struct coroutine_context {
    void* rbx;
    void* rsp;
    void* rbp;
    void* r12;
    void* r13;
    void* r14;
    void* r15;
    void* rip;
};

// 协程状态
enum coroutine_state {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

// 协程类
class coroutine {
public:
    coroutine(std::function<void()> func)
        : func_(func), state_(READY), context_{} {
        // 分配协程栈 (128KB)
        stack_size_ = 128 * 1024;
        stack_ = new char[stack_size_];
        prepare_context();
    }

    ~coroutine() {
        delete[] stack_;
    }

    void prepare_context() {
        char* stack_top = stack_ + stack_size_;
        
        // x86_64 栈需要保持16字节对齐
        stack_top = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(stack_top) & ~15));
        
        // 设置RIP指向协程入口函数
        context_.rip = reinterpret_cast<void*>(&entry_point_wrapper);
        
        // 设置RSP指向栈顶
        context_.rsp = stack_top;
        
        // 存储this指针到栈上
        *reinterpret_cast<coroutine**>(stack_top - sizeof(coroutine*)) = this;
        context_.rsp = stack_top - sizeof(coroutine*);
    }

    static void entry_point_wrapper() {
        coroutine* co = get_current_coroutine();
        co->func_();
        co->state_ = FINISHED;
        co->yield();
    }

    static coroutine* get_current_coroutine() {
        return *reinterpret_cast<coroutine**>(
            reinterpret_cast<char*>(&get_current_coroutine) + sizeof(void*)
        );
    }

    void yield() {
        // 保存当前上下文并切换回调度器
        swap_context(&context_, scheduler_->context_);
    }

    coroutine_state state() const { return state_; }
    void set_state(coroutine_state state) { state_ = state; }

    static void set_current(coroutine* co) { current = co; }
    static coroutine* current;
    class scheduler* scheduler_ = nullptr;
    
private:
    static void swap_context(coroutine_context* curr, coroutine_context* next);
    
    std::function<void()> func_;
    coroutine_context context_;
    char* stack_;
    size_t stack_size_;
    coroutine_state state_;
};

coroutine* coroutine::current = nullptr;

// x86-64 上下文切换汇编实现
extern "C" void swap_context(coroutine_context* curr, coroutine_context* next) asm("swap_context");

asm(
".globl swap_context\n"
"swap_context:\n"
"    pushq %rbp\n"
"    movq %rsp, %rbp\n"
"    movq (%rbp), %rax\n"        // 保存返回地址
"    movq %rax, 0x38(%rdi)\n"    // 存到 curr->rip
    
"    movq %rbx, 0x00(%rdi)\n"
"    movq %rsp, 0x08(%rdi)\n"
"    movq %rbp, 0x10(%rdi)\n"
"    movq %r12, 0x18(%rdi)\n"
"    movq %r13, 0x20(%rdi)\n"
"    movq %r14, 0x28(%rdi)\n"
"    movq %r15, 0x30(%rdi)\n"
    
"    movq 0x00(%rsi), %rbx\n"    // 恢复新上下文
"    movq 0x08(%rsi), %rsp\n"
"    movq 0x10(%rsi), %rbp\n"
"    movq 0x18(%rsi), %r12\n"
"    movq 0x20(%rsi), %r13\n"
"    movq 0x28(%rsi), %r14\n"
"    movq 0x30(%rsi), %r15\n"
    
"    movq 0x38(%rsi), %rax\n"    // 加载新RIP
"    pushq %rax\n"               // 压入栈作为返回地址
"    ret\n"                      // 返回到新地址
);

// 调度器类
class scheduler {
public:
    scheduler(int id) 
        : id_(id), quit_(false) {
        context_ = new coroutine_context;
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1) {
            perror("epoll_create");
            exit(EXIT_FAILURE);
        }
    }
    
    ~scheduler() {
        delete context_;
        if (epoll_fd_ != -1) close(epoll_fd_);
    }

    void run() {
        // 绑定线程到特定核心
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(id_, &cpuset);
        
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
            perror("pthread_setaffinity_np");
        }
        
        // 启动epoll事件处理线程
        event_thread_ = std::thread(&scheduler::event_loop, this);
        
        while (!quit_) {
            coroutine* co = get_next_coroutine();
            if (co) {
                co->set_state(RUNNING);
                co->scheduler_ = this;
                coroutine::set_current(co);
                swap_context(context_, &co->context_);
                coroutine::set_current(nullptr);
                
                if (co->state() == FINISHED) {
                    delete co;
                }
            } else {
                // 工作队列为空时暂停CPU
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    void stop() {
        quit_ = true;
        if (event_thread_.joinable()) {
            event_thread_.join();
        }
    }

    void schedule(coroutine* co) {
        std::lock_guard<std::mutex> lock(work_queue_mutex_);
        work_queue_.push(co);
    }

    void register_io(int fd, coroutine* co) {
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            perror("epoll_ctl");
            return;
        }
        
        std::lock_guard<std::mutex> lock(io_mutex_);
        waiting_coros_[fd] = co;
    }

private:
    coroutine* get_next_coroutine() {
        std::lock_guard<std::mutex> lock(work_queue_mutex_);
        if (work_queue_.empty()) {
            return nullptr;
        }
        
        coroutine* co = work_queue_.front();
        work_queue_.pop();
        return co;
    }

    void event_loop() {
        const int MAX_EVENTS = 64;
        epoll_event events[MAX_EVENTS];
        
        while (!quit_) {
            int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 10);
            if (n < 0) {
                if (errno == EINTR) continue;
                perror("epoll_wait");
                break;
            }
            
            for (int i = 0; i < n; i++) {
                int fd = events[i].data.fd;
                
                std::lock_guard<std::mutex> lock(io_mutex_);
                auto it = waiting_coros_.find(fd);
                if (it != waiting_coros_.end()) {
                    coroutine* co = it->second;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
                    waiting_coros_.erase(it);
                    
                    co->set_state(READY);
                    {
                        std::lock_guard<std::mutex> lock_q(work_queue_mutex_);
                        work_queue_.push(co);
                    }
                }
            }
        }
    }
    
    int id_;
    coroutine_context* context_;
    int epoll_fd_ = -1;
    std::thread event_thread_;
    
    std::queue<coroutine*> work_queue_;
    std::mutex work_queue_mutex_;
    
    std::unordered_map<int, coroutine*> waiting_coros_;
    std::mutex io_mutex_;
    
    std::atomic<bool> quit_;
};

// 多核调度器管理
class scheduler_manager {
public:
    static scheduler_manager& instance() {
        static scheduler_manager inst;
        return inst;
    }

    void start(int num_schedulers) {
        int cores = std::thread::hardware_concurrency();
        if (num_schedulers > 0 && num_schedulers <= cores) {
            cores = num_schedulers;
        }
        
        schedulers_.reserve(cores);
        threads_.reserve(cores);
        
        for (int i = 0; i < cores; i++) {
            schedulers_.emplace_back(new scheduler(i));
            threads_.emplace_back([sched = schedulers_.back().get()] {
                sched->run();
            });
        }
    }

    void stop() {
        for (auto& sched : schedulers_) {
            sched->stop();
        }
        
        for (auto& thread : threads_) {
            if (thread.joinable()) thread.join();
        }
        
        schedulers_.clear();
        threads_.clear();
    }

    void schedule(coroutine* co) {
        // 简单的轮询分配
        static std::atomic<size_t> index = 0;
        size_t i = index++ % schedulers_.size();
        schedulers_[i]->schedule(co);
    }

private:
    scheduler_manager() = default;
    ~scheduler_manager() { stop(); }
    
    std::vector<std::unique_ptr<scheduler>> schedulers_;
    std::vector<std::thread> threads_;
};

// 协程友好IO函数
int co_read(int fd, char* buf, size_t n) {
    while (true) {
        ssize_t count = read(fd, buf, n);
        
        // 成功读取或发生永久错误
        if (count > 0 || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            return count;
        }
        
        // 协程挂起逻辑
        auto co = coroutine::current;
        if (co && co->scheduler_) {
            co->set_state(WAITING);
            co->scheduler_->register_io(fd, co);
            co->yield();
        }
    }
}

// 设置非阻塞IO
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
    }
}

// 创建TCP服务器
int create_tcp_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // 允许端口复用
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) {
        perror("setsockopt");
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 1000) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    set_nonblocking(server_fd);
    return server_fd;
}

// 示例协程函数 - TCP回显服务器
void client_handler(int client_fd) {
    char buf[1024];
    while (true) {
        ssize_t n = co_read(client_fd, buf, sizeof(buf));
        if (n <= 0) {
            // 连接关闭或错误
            if (n < 0) {
                perror("read");
            }
            close(client_fd);
            break;
        }
        
        // 回显数据
        write(client_fd, buf, n);
    }
}

// 接受连接的协程函数
void acceptor_function(int server_fd) {
    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 在协程中等待事件
                auto co = coroutine::current;
                if (co && co->scheduler_) {
                    co->set_state(WAITING);
                    co->scheduler_->register_io(server_fd, co);
                    co->yield();
                }
            } else {
                perror("accept");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } else {
            set_nonblocking(client_fd);
            std::cout << "Accepted new connection: " << client_fd << std::endl;
            
            // 创建新协程处理客户端
            coroutine* client_co = new coroutine([client_fd] {
                client_handler(client_fd);
            });
            scheduler_manager::instance().schedule(client_co);
        }
    }
}

int main() {
    std::cout << "Starting multi-core coroutine framework..." << std::endl;
    std::cout << "Detected cores: " << std::thread::hardware_concurrency() << std::endl;
    
    // 启动多核调度器
    scheduler_manager::instance().start(0); // 0 = auto-detect cores
    
    // 创建TCP服务器
    int server_fd = create_tcp_server(8080);
    std::cout << "TCP server listening on port 8080..." << std::endl;
    
    // 创建接受连接的协程
    coroutine* acceptor_co = new coroutine([server_fd] {
        acceptor_function(server_fd);
    });
    scheduler_manager::instance().schedule(acceptor_co);
    
    // 主线程保持运行，等待Ctrl+C
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // 清理
    close(server_fd);
    scheduler_manager::instance().stop();
    return 0;
}