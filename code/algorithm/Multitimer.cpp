#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <memory>
#include<chrono>
class MultiTimer {
private:
    struct TimerInfo {
        int fd;
        std::function<void()> callback;
        std::string name;
    };
    
    int epoll_fd;
    std::unordered_map<int, std::unique_ptr<TimerInfo>> timers; // fd -> TimerInfo
    
public:
    MultiTimer() : epoll_fd(-1) {
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw std::runtime_error("epoll_create1 failed");
        }
    }
    
    ~MultiTimer() {
        stop_all();
        if (epoll_fd != -1) {
            close(epoll_fd);
        }
    }
    
    // 添加定时器
    bool add_timer(const std::string& name, int interval_ms, std::function<void()> cb) {
        // 创建 timerfd
        int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timer_fd == -1) {
            perror("timerfd_create failed");
            return false;
        }
        
        // 设置定时器
        struct itimerspec timer_spec;
        timer_spec.it_interval.tv_sec = interval_ms / 1000;
        timer_spec.it_interval.tv_nsec = (interval_ms % 1000) * 1000000;
        timer_spec.it_value = timer_spec.it_interval;
        
        if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
            perror("timerfd_settime failed");
            close(timer_fd);
            return false;
        }
        
        // 添加到 epoll
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = timer_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
            perror("epoll_ctl failed");
            close(timer_fd);
            return false;
        }
        
        // 保存定时器信息
        auto timer_info = std::make_unique<TimerInfo>();
        timer_info->fd = timer_fd;
        timer_info->callback = cb;
        timer_info->name = name;
        
        timers[timer_fd] = std::move(timer_info);
        
        std::cout << "添加定时器: " << name << ", 间隔: " << interval_ms << "ms" << std::endl;
        return true;
    }
    
    // 移除定时器
    void remove_timer(const std::string& name) {
        for (auto it = timers.begin(); it != timers.end(); ) {
            if (it->second->name == name) {
                std::cout << "移除定时器: " << name << std::endl;
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
                close(it->first);
                it = timers.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // 运行事件循环（带超时）
    void run(int timeout_ms = -1) {
        struct epoll_event events[10];
        
        while (!timers.empty()) {
            int nfds = epoll_wait(epoll_fd, events, 10, timeout_ms);
            
            if (nfds == -1) {
                if (errno == EINTR) continue;
                perror("epoll_wait failed");
                break;
            }
            
            if (nfds == 0) {
                std::cout << "epoll_wait 超时" << std::endl;
                continue;
            }
            
            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;
                auto it = timers.find(fd);
                
                if (it != timers.end() && (events[i].events & EPOLLIN)) {
                    handle_timer(it->second.get());
                }
            }
        }
    }
    
    // 停止所有定时器
    void stop_all() {
        for (auto& pair : timers) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pair.first, NULL);
            close(pair.first);
        }
        timers.clear();
        std::cout << "所有定时器已停止" << std::endl;
    }
    
    // 获取活跃定时器数量
    size_t count() const {
        return timers.size();
    }
    
private:
    void handle_timer(TimerInfo* timer) {
        // 读取超时次数
        uint64_t expirations;
        if (read(timer->fd, &expirations, sizeof(expirations)) != sizeof(expirations)) {
            perror("read timerfd failed");
            return;
        }
        
        std::cout << "定时器 '" << timer->name << "' 触发, 次数: " << expirations << std::endl;
        
        if (timer->callback) {
            timer->callback();
        }
    }
};// 测试多个定时器
int main() {
    try {
        MultiTimer timer_mgr;
        
        // 添加快速定时器（每秒2次）
        timer_mgr.add_timer("fast", 500, []() {
            static int count = 0;
            std::cout << "  快速定时器工作... (" << ++count << ")" << std::endl;
        });
        
        // 添加慢速定时器（每3秒1次）
        timer_mgr.add_timer("slow", 3000, []() {
            static int count = 0;
            std::cout << "  === 慢速定时器工作 === (" << ++count << ")" << std::endl;
        });
        
        // 添加一次性定时器（10秒后移除自己）
        timer_mgr.add_timer("oneshot", 10000, [&timer_mgr]() {
            std::cout << "  *** 一次性定时器触发，即将自移除 ***" << std::endl;
            timer_mgr.remove_timer("oneshot");
        });
        
        std::cout << "启动 " << timer_mgr.count() << " 个定时器..." << std::endl;
        std::cout << "运行30秒后自动退出..." << std::endl;
        
        // 运行30秒
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(30)) {
            timer_mgr.run(1000); // 1秒超时，可以在此期间处理其他任务
        }
        
        std::cout << "30秒到达，程序退出" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
