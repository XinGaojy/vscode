#ifndef FLARE_SCHEDULER_H
#define FLARE_SCHEDULER_H

#include "fiber.h"
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
namespace flare {

class Scheduler {
public:
    // 获取全局单例
    static Scheduler& instance() {
        static Scheduler sched;
        return sched;
    }
    
    // 添加新纤程
    void spawn(Fiber::Func f) {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        _ready_queue.push(std::make_unique<Fiber>(std::move(f)));
        _idle_workers.notify_one();
    }
    
    // 运行调度器（阻塞调用）
    void run() {
        _running = true;
        const size_t worker_count = std::thread::hardware_concurrency();
        _workers.reserve(worker_count);
        
        for (size_t i = 0; i < worker_count; ++i) {
            _workers.emplace_back([this] { worker_loop(); });
        }
        
        for (auto& t : _workers) {
            if (t.joinable()) t.join();
        }
    }
    
    // 停止调度器
    void stop() {
        _running = false;
        _idle_workers.notify_all();
    }

private:
    Scheduler() = default;
    
    // 工作线程主循环
    void worker_loop() {
        while (_running) {
            std::unique_ptr<Fiber> fiber;
            {
                std::unique_lock<std::mutex> lock(_queue_mutex);
                _idle_workers.wait(lock, [this] {
                    return !_running || !_ready_queue.empty();
                });
                
                if (!_running) break;
                
                if (!_ready_queue.empty()) {
                    fiber = std::move(_ready_queue.front());
                    _ready_queue.pop();
                }
            }
            
            if (fiber) {
                fiber->resume();
                
                if (!fiber->finished()) {
                    std::lock_guard<std::mutex> lock(_queue_mutex);
                    _ready_queue.push(std::move(fiber));
                }
            }
        }
    }
    
    std::vector<std::thread> _workers;
    std::queue<std::unique_ptr<Fiber>> _ready_queue;
    std::mutex _queue_mutex;
    std::condition_variable _idle_workers;
    std::atomic<bool> _running{false};
};

} // namespace flare

#endif // FLARE_SCHEDULER_H
