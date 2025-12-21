//代码有问题,需要debug


#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>
#include <deque>
#include <functional>
#include <atomic>
#include <queue>
#include <thread>
#include <future>
#include <type_traits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <condition_variable>
#include <sstream>
#include <cassert>

namespace ThreadPool {
    
// 任务优先级定义
enum class TaskPriority {
    HIGHEST = 0,    // 最高优先级
    HIGH = 1,       // 高优先级
    NORMAL = 2,     // 普通优先级
    LOW = 3,        // 低优先级
    LOWEST = 4      // 最低优先级
};

// 任务取消令牌
class TaskCancellationSource {
private:
    std::atomic<bool> cancelled_{false};
    
public:
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }
    bool is_cancelled() const noexcept { return cancelled_.load(std::memory_order_acquire); }
};

using TaskCancellationToken = std::shared_ptr<TaskCancellationSource>;

// 线程池统计信息
struct ThreadPoolStats {
    std::atomic<size_t> total_tasks_submitted{0};      // 总提交任务数
    std::atomic<size_t> total_tasks_completed{0};       // 总完成任务数
    std::atomic<size_t> tasks_pending{0};              // 等待中任务数
    std::atomic<size_t> tasks_executing{0};             // 执行中任务数
    std::atomic<size_t> tasks_cancelled{0};            // 取消的任务数
    std::atomic<size_t> tasks_timed_out{0};            // 超时的任务数
    std::chrono::steady_clock::time_point start_time;  // 启动时间
    
    ThreadPoolStats() : start_time(std::chrono::steady_clock::now()) {}
    
    // 计算运行时间
    std::chrono::duration<double> uptime() const {
        return std::chrono::steady_clock::now() - start_time;
    }
    
    // 获取平均任务处理速度（任务/秒）
    double tasks_per_second() const {
        auto elapsed = uptime();
        if (elapsed.count() > 0) {
            return total_tasks_completed.load() / elapsed.count();
        }
        return 0.0;
    }
};

// 线程本地存储包装器
template<typename T>
class ThreadLocal {
private:
    struct ThreadData {
        std::thread::id thread_id;
        std::unique_ptr<T> data;
    };
    
    std::vector<ThreadData> data_list_;
    mutable std::shared_mutex mutex_;
    
public:
    T& get() {
        auto id = std::this_thread::get_id();
        {
            std::shared_lock lock(mutex_);
            for (auto& td : data_list_) {
                if (td.thread_id == id && td.data) {
                    return *td.data;
                }
            }
        }
        
        {
            std::unique_lock lock(mutex_);
            // 再次检查，防止多个线程同时创建
            for (auto& td : data_list_) {
                if (td.thread_id == id && td.data) {
                    return *td.data;
                }
            }
            // 创建新的线程本地数据
            data_list_.push_back({id, std::make_unique<T>()});
            return *data_list_.back().data;
        }
    }
    
    void clear() {
        std::unique_lock lock(mutex_);
        data_list_.clear();
    }
    
    // 清理已退出线程的数据
    void cleanup() {
        auto id = std::this_thread::get_id();
        std::unique_lock lock(mutex_);
        data_list_.erase(
            std::remove_if(data_list_.begin(), data_list_.end(),
                [&](const ThreadData& td) {
                    return td.thread_id != id && td.thread_id != std::thread::id{};
                }),
            data_list_.end()
        );
    }
};

// 线程池配置
struct ThreadPoolConfig {
    size_t min_threads;              // 最小线程数
    size_t max_threads;              // 最大线程数
    size_t max_queue_size;           // 最大队列大小 (0=无限制)
    size_t idle_timeout_ms;          // 空闲线程超时时间(毫秒)
    size_t max_idle_threads;         // 最大空闲线程数
    bool enable_work_stealing;       // 是否启用工作窃取
    bool auto_shrink;                // 是否自动收缩
    bool enable_statistics;          // 是否启用统计
    
    ThreadPoolConfig() 
        : min_threads(std::thread::hardware_concurrency())
        , max_threads(std::thread::hardware_concurrency() * 2)
        , max_queue_size(1000)
        , idle_timeout_ms(60000)    // 60秒
        , max_idle_threads(std::thread::hardware_concurrency())
        , enable_work_stealing(true)
        , auto_shrink(true)
        , enable_statistics(true) {}
    
    // 验证配置有效性
    bool validate() const {
        if (min_threads == 0) return false;
        if (max_threads < min_threads) return false;
        if (idle_timeout_ms == 0) return false;
        if (max_idle_threads == 0) return false;
        return true;
    }
};

// 带优先级的任务包装器
class PriorityTask {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    
private:
    int64_t sequence_;                            // 任务序列号
    TaskPriority priority_;                       // 任务优先级
    std::function<void()> func_;                  // 实际任务函数
    TaskCancellationToken cancellation_token_;    // 取消令牌
    std::optional<TimePoint> deadline_;           // 截止时间
    std::string name_;                            // 任务名称（用于调试）
    ThreadLocal<void*>* thread_local_storage_;     // 线程本地存储指针
    
public:
    PriorityTask(int64_t seq, TaskPriority prio, std::function<void()> func,
                const std::string& name = "")
        : sequence_(seq)
        , priority_(prio)
        , func_(std::move(func))
        , name_(name)
        , thread_local_storage_(nullptr) {}
    
    // 比较函数，用于优先队列
    bool operator<(const PriorityTask& other) const {
        if (priority_ != other.priority_) {
            return static_cast<int>(priority_) > static_cast<int>(other.priority_);
        }
        if (deadline_.has_value() != other.deadline_.has_value()) {
            return !deadline_.has_value();  // 有截止时间的优先
        }
        if (deadline_.has_value() && other.deadline_.has_value()) {
            if (*deadline_ != *other.deadline_) {
                return *deadline_ > *other.deadline_;  // 截止时间早的优先
            }
        }
        return sequence_ > other.sequence_;  // 序列号小的优先
    }
    
    // 执行任务
    void operator()() {
        if (cancellation_token_ && cancellation_token_->is_cancelled()) {
            return;
        }
        
        if (deadline_.has_value() && Clock::now() > *deadline_) {
            return;
        }
        
        try {
            func_();
        } catch (...) {
            // 异常处理交给调用者
            throw;
        }
    }
    
    // 设置属性
    void set_cancellation_token(TaskCancellationToken token) { 
        cancellation_token_ = std::move(token); 
    }
    
    void set_deadline(TimePoint deadline) { 
        deadline_ = deadline; 
    }
    
    void set_name(const std::string& name) { 
        name_ = name; 
    }
    
    void set_thread_local_storage(ThreadLocal<void*>* tls) { 
        thread_local_storage_ = tls; 
    }
    
    // 获取属性
    TaskPriority priority() const { return priority_; }
    const std::string& name() const { return name_; }
    bool has_deadline() const { return deadline_.has_value(); }
    TimePoint deadline() const { return deadline_.value(); }
};

// 主线程池类
class ThreadPool {
private:
    // 任务包装器，支持返回值和异常
    template<typename T>
    class TaskWrapper {
    private:
        std::packaged_task<T()> task_;
        PriorityTask priority_task_;
        
    public:
        template<typename Func>
        TaskWrapper(int64_t seq, TaskPriority prio, Func&& func, const std::string& name = "")
            : task_(std::forward<Func>(func))
            , priority_task_(seq, prio, [this] { task_(); }, name) {}
        
        PriorityTask& priority_task() { return priority_task_; }
        std::future<T> get_future() { return task_.get_future(); }
    };
    
    // 任务队列类型定义
    using TaskQueue = std::priority_queue<PriorityTask>;
    
private:
    ThreadPoolConfig config_;                      // 配置
    mutable std::shared_mutex config_mutex_;        // 配置修改锁
    
    std::vector<std::thread> workers_;              // 工作线程
    TaskQueue task_queue_;                          // 任务队列
    mutable std::mutex queue_mutex_;                // 任务队列锁
    std::condition_variable queue_cv_;              // 任务队列条件变量
    std::condition_variable not_full_cv_;           // 队列不满条件变量
    
    std::atomic<bool> stop_{false};                 // 停止标志
    std::atomic<bool> shutdown_requested_{false};   // 关闭请求标志
    std::atomic<int64_t> task_sequence_{0};         // 任务序列号生成器
    
    ThreadPoolStats stats_;                         // 统计信息
    std::atomic<size_t> active_workers_{0};         // 活动工作线程数
    std::atomic<size_t> idle_workers_{0};          // 空闲工作线程数
    
    ThreadLocal<void*> thread_local_storage_;       // 线程本地存储
    
    // 工作窃取相关
    std::vector<std::deque<std::function<void()>>> work_stealing_queues_;
    mutable std::vector<std::mutex> work_stealing_mutexes_;
    
public:
    explicit ThreadPool(const ThreadPoolConfig& config = ThreadPoolConfig())
        : config_(config) {
        
        if (!config_.validate()) {
            throw std::invalid_argument("Invalid thread pool configuration");
        }
        
        if (config_.enable_work_stealing) {
            work_stealing_queues_.resize(config_.max_threads);
            work_stealing_mutexes_.resize(config_.max_threads);
        }
        
        start_workers(config_.min_threads);
    }
    
    ~ThreadPool() {
        shutdown();
    }
    
    // 禁用拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // 移动支持
    ThreadPool(ThreadPool&&) = default;
    ThreadPool& operator=(ThreadPool&&) = default;
    
private:
    // 启动工作线程
    void start_workers(size_t num) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        for (size_t i = 0; i < num && workers_.size() < config_.max_threads; ++i) {
            workers_.emplace_back([this, worker_id = workers_.size()] {
                worker_thread(worker_id);
            });
        }
    }
    
    // 工作线程主函数
    void worker_thread(size_t worker_id) {
        // 设置线程名称（用于调试）
        std::ostringstream oss;
        oss << "TP-Worker-" << worker_id;
        set_thread_name(oss.str());
        
        // 初始化线程本地存储
        thread_local_storage_.get() = nullptr;
        
        bool is_idle = false;
        auto last_work_time = std::chrono::steady_clock::now();
        
        while (true) {
            std::function<void()> task;
            bool should_stop = false;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                
                // 更新空闲状态
                if (is_idle) {
                    idle_workers_--;
                }
                is_idle = true;
                idle_workers_++;
                
                // 等待任务或停止信号
                auto predicate = [this] {
                    return stop_.load(std::memory_order_acquire) || 
                           !task_queue_.empty() ||
                           (config_.enable_work_stealing && has_work_in_work_stealing_queue());
                };
                
                if (config_.idle_timeout_ms > 0 && config_.auto_shrink) {
                    // 带超时的等待
                    auto status = queue_cv_.wait_for(lock, 
                        std::chrono::milliseconds(config_.idle_timeout_ms), predicate);
                    
                    if (!status && 
                        workers_.size() > config_.min_threads && 
                        idle_workers_ > config_.max_idle_threads &&
                        !stop_) {
                        // 线程空闲超时，且线程数超过最小值，可以退出
                        should_stop = true;
                    }
                } else {
                    queue_cv_.wait(lock, predicate);
                }
                
                if (should_stop) {
                    // 从线程列表中移除自己
                    auto it = std::find_if(workers_.begin(), workers_.end(),
                        [thread_id = std::this_thread::get_id()](const std::thread& t) {
                            return t.get_id() == thread_id;
                        });
                    
                    if (it != workers_.end()) {
                        it->detach();
                        workers_.erase(it);
                        idle_workers_--;
                    }
                    thread_local_storage_.cleanup();
                    return;
                }
                
                if (stop_.load(std::memory_order_acquire) && task_queue_.empty()) {
                    idle_workers_--;
                    thread_local_storage_.cleanup();
                    return;
                }
                
                // 获取任务
                if (!task_queue_.empty()) {
                    task = std::move(const_cast<PriorityTask&>(task_queue_.top()).func_);
                    task_queue_.pop();
                    not_full_cv_.notify_one();
                } else if (config_.enable_work_stealing) {
                    task = try_steal_work(worker_id);
                }
                
                if (task) {
                    // 更新状态
                    is_idle = false;
                    idle_workers_--;
                    stats_.tasks_pending.fetch_sub(1, std::memory_order_relaxed);
                    stats_.tasks_executing.fetch_add(1, std::memory_order_relaxed);
                    last_work_time = std::chrono::steady_clock::now();
                } else {
                    // 继续等待
                    continue;
                }
            }
            
            // 执行任务
            if (task) {
                try {
                    task();
                    stats_.total_tasks_completed.fetch_add(1, std::memory_order_relaxed);
                } catch (...) {
                    // 记录异常但不影响线程池运行
                }
                stats_.tasks_executing.fetch_sub(1, std::memory_order_relaxed);
            }
        }
    }
    
    // 尝试从其他线程窃取工作
    std::function<void()> try_steal_work(size_t thief_id) {
        if (!config_.enable_work_stealing) {
            return nullptr;
        }
        
        for (size_t i = 0; i < work_stealing_queues_.size(); ++i) {
            if (i == thief_id) continue;  // 不窃取自己的任务
            
            std::unique_lock<std::mutex> lock(work_stealing_mutexes_[i], 
                                             std::try_to_lock);
            if (lock.owns_lock() && !work_stealing_queues_[i].empty()) {
                auto task = std::move(work_stealing_queues_[i].back());
                work_stealing_queues_[i].pop_back();
                return task;
            }
        }
        return nullptr;
    }
    
    // 检查工作窃取队列中是否有任务
    bool has_work_in_work_stealing_queue() const {
        for (const auto& queue : work_stealing_queues_) {
            if (!queue.empty()) {
                return true;
            }
        }
        return false;
    }
    
    // 设置线程名称（平台相关）
    void set_thread_name(const std::string& name) {
        // 这里可以添加平台相关的线程命名代码
        // Linux: pthread_setname_np
        // Windows: SetThreadDescription
        // macOS: pthread_setname_np
    }
    
public:
    // 提交任务（无返回值）
    template<typename Func, typename... Args>
    void submit(TaskPriority priority, const std::string& name, Func&& func, Args&&... args) {
        submit_impl(priority, name, 
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    }
    
    // 提交任务（有返回值）
    template<typename Func, typename... Args>
    auto submit_with_result(TaskPriority priority, const std::string& name, 
                           Func&& func, Args&&... args) 
        -> std::future<typename std::invoke_result_t<Func, Args...>> {
        
        using ReturnType = typename std::invoke_result_t<Func, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        submit_impl(priority, name, [task]() { (*task)(); });
        
        return result;
    }
    
    // 提交延迟任务
    template<typename Func, typename... Args>
    void submit_after(TaskPriority priority, const std::string& name,
                     std::chrono::milliseconds delay,
                     Func&& func, Args&&... args) {
        
        auto scheduled_func = [=]() mutable {
            std::this_thread::sleep_for(delay);
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        };
        
        submit_impl(priority, name, std::move(scheduled_func));
    }
    
    // 提交定时任务
    template<typename Func, typename... Args>
    void submit_at(TaskPriority priority, const std::string& name,
                  std::chrono::steady_clock::time_point time_point,
                  Func&& func, Args&&... args) {
        
        auto scheduled_func = [=]() mutable {
            auto now = std::chrono::steady_clock::now();
            if (time_point > now) {
                std::this_thread::sleep_for(time_point - now);
            }
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        };
        
        submit_impl(priority, name, std::move(scheduled_func));
    }
    
    // 提交带取消令牌的任务
    template<typename Func, typename... Args>
    std::pair<std::future<typename std::invoke_result_t<Func, Args...>>, 
              TaskCancellationToken>
    submit_with_cancellation(TaskPriority priority, const std::string& name,
                           Func&& func, Args&&... args) {
        
        using ReturnType = typename std::invoke_result_t<Func, Args...>;
        
        auto cancellation_source = std::make_shared<TaskCancellationSource>();
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<Func>(func), 
             args = std::make_tuple(std::forward<Args>(args)...),
             token = cancellation_source]() mutable -> ReturnType {
                
                if (token && token->is_cancelled()) {
                    throw std::runtime_error("Task cancelled");
                }
                
                return std::apply(func, args);
            }
        );
        
        std::future<ReturnType> result = task->get_future();
        
        auto wrapped_task = [task, token = cancellation_source]() {
            if (!token->is_cancelled()) {
                (*task)();
            }
        };
        
        submit_impl(priority, name, std::move(wrapped_task), cancellation_source);
        
        return {std::move(result), std::move(cancellation_source)};
    }
    
    // 快捷方法
    template<typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args) {
        return submit_with_result(TaskPriority::NORMAL, "", 
                                 std::forward<Func>(func), std::forward<Args>(args)...);
    }
    
    template<typename Func, typename... Args>
    void submit_high_priority(Func&& func, Args&&... args) {
        submit(TaskPriority::HIGH, "", std::forward<Func>(func), std::forward<Args>(args)...);
    }
    
    // 等待所有任务完成
    void wait_all() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        not_full_cv_.wait(lock, [this] {
            return task_queue_.empty() && 
                   stats_.tasks_executing.load() == 0 &&
                   (!config_.enable_work_stealing || !has_work_in_work_stealing_queue());
        });
    }
    
    // 等待特定任务完成
    template<typename... Futures>
    void wait_any(Futures&&... futures) {
        std::vector<std::future<void>> all_futures = {futures.share()...};
        while (true) {
            for (auto& fut : all_futures) {
                if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    return;
                }
            }
            std::this_thread::yield();
        }
    }
    
    template<typename... Futures>
    void wait_all(Futures&&... futures) {
        (futures.wait(), ...);
    }
    
    // 优雅关闭
    void shutdown() {
        if (shutdown_requested_.exchange(true)) {
            return;  // 已经在关闭中
        }
        
        // 等待所有任务完成
        wait_all();
        
        // 停止线程
        stop_.store(true, std::memory_order_release);
        queue_cv_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers_.clear();
    }
    
    // 立即关闭（不等待任务完成）
    void shutdown_now() {
        stop_.store(true, std::memory_order_release);
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            // 清空任务队列
            task_queue_ = TaskQueue();
        }
        
        queue_cv_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.detach();  // 立即分离，不等待
            }
        }
        workers_.clear();
    }
    
    // 获取统计信息
    ThreadPoolStats get_stats() const {
        ThreadPoolStats copy = stats_;
        copy.tasks_pending = task_queue_.size();
        return copy;
    }
    
    // 调整线程池大小
    void resize(size_t num_threads) {
        if (num_threads < config_.min_threads || num_threads > config_.max_threads) {
            throw std::invalid_argument("Thread count out of range");
        }
        
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (num_threads > workers_.size()) {
            start_workers(num_threads - workers_.size());
        } else if (num_threads < workers_.size()) {
            // 唤醒多余线程，让它们自然退出
            queue_cv_.notify_all();
        }
    }
    
    // 获取线程本地存储
    template<typename T>
    T& get_thread_local() {
        return *reinterpret_cast<T*>(thread_local_storage_.get());
    }
    
    template<typename T>
    void set_thread_local(T* ptr) {
        thread_local_storage_.get() = ptr;
    }
    
    // 获取线程池状态
    std::string get_status_string() const {
        std::ostringstream oss;
        oss << "ThreadPool Status:\n";
        oss << "  Threads: " << workers_.size() << " active, "
            << idle_workers_.load() << " idle\n";
        oss << "  Tasks: " << stats_.tasks_pending.load() << " pending, "
            << stats_.tasks_executing.load() << " executing\n";
        oss << "  Completed: " << stats_.total_tasks_completed.load() << "\n";
        oss << "  Uptime: " << stats_.uptime().count() << " seconds\n";
        oss << "  Throughput: " << stats_.tasks_per_second() << " tasks/second\n";
        return oss.str();
    }
    
private:
    // 提交任务实现
    void submit_impl(TaskPriority priority, const std::string& name,
                     std::function<void()> func,
                     TaskCancellationToken token = nullptr) {
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // 检查队列是否已满
            if (config_.max_queue_size > 0 && 
                task_queue_.size() >= config_.max_queue_size) {
                throw std::runtime_error("Task queue is full");
            }
            
            // 创建任务
            auto task = PriorityTask(
                task_sequence_.fetch_add(1, std::memory_order_relaxed),
                priority,
                std::move(func),
                name
            );
            
            task.set_cancellation_token(token);
            task.set_thread_local_storage(&thread_local_storage_);
            
            task_queue_.push(std::move(task));
            stats_.total_tasks_submitted.fetch_add(1, std::memory_order_relaxed);
            stats_.tasks_pending.fetch_add(1, std::memory_order_relaxed);
        }
        
        // 通知工作线程
        queue_cv_.notify_one();
        
        // 如果需要，创建新线程
        if (should_create_new_worker()) {
            start_workers(1);
        }
    }
    
    // 检查是否需要创建新线程
    bool should_create_new_worker() const {
        if (workers_.size() >= config_.max_threads) {
            return false;
        }
        
        // 如果有等待任务，且没有空闲线程，创建新线程
        if (stats_.tasks_pending.load() > 0 && idle_workers_.load() == 0) {
            return true;
        }
        
        return false;
    }
};

// 单例线程池管理器
class ThreadPoolManager {
private:
    static ThreadPoolManager* instance_;
    static std::mutex instance_mutex_;
    
    std::unordered_map<std::string, std::unique_ptr<ThreadPool>> pools_;
    mutable std::shared_mutex pools_mutex_;
    
    ThreadPoolManager() = default;
    
public:
    static ThreadPoolManager& get_instance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = new ThreadPoolManager();
        }
        return *instance_;
    }
    
    ThreadPool& get_pool(const std::string& name = "default") {
        std::shared_lock lock(pools_mutex_);
        auto it = pools_.find(name);
        if (it != pools_.end()) {
            return *it->second;
        }
        
        lock.unlock();
        std::unique_lock unique_lock(pools_mutex_);
        
        // 再次检查（双重检查锁定）
        it = pools_.find(name);
        if (it != pools_.end()) {
            return *it->second;
        }
        
        // 创建新的线程池
        ThreadPoolConfig config;
        if (name == "io") {
            config.min_threads = 4;
            config.max_threads = 16;
            config.max_queue_size = 10000;
        } else if (name == "compute") {
            config.min_threads = std::thread::hardware_concurrency();
            config.max_threads = std::thread::hardware_concurrency() * 2;
            config.max_queue_size = 1000;
        }
        
        auto [new_it, inserted] = pools_.emplace(name, std::make_unique<ThreadPool>(config));
        return *new_it->second;
    }
    
    void shutdown_all() {
        std::unique_lock lock(pools_mutex_);
        for (auto& [name, pool] : pools_) {
            pool->shutdown();
        }
        pools_.clear();
    }
    
    std::vector<std::string> get_pool_names() const {
        std::shared_lock lock(pools_mutex_);
        std::vector<std::string> names;
        for (const auto& [name, _] : pools_) {
            names.push_back(name);
        }
        return names;
    }
    
    ~ThreadPoolManager() {
        shutdown_all();
    }
};

// 初始化静态成员
ThreadPoolManager* ThreadPoolManager::instance_ = nullptr;
std::mutex ThreadPoolManager::instance_mutex_;

// 全局便捷函数
inline ThreadPool& get_thread_pool(const std::string& name = "default") {
    return ThreadPoolManager::get_instance().get_pool(name);
}

} // namespace ThreadPool

// 使用示例
int main() {
    using namespace ThreadPool;
    
    try {
        // 1. 基本使用
        {
            std::cout << "=== 基本使用示例 ===" << std::endl;
            ThreadPool pool;
            
            auto result1 = pool.submit([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return 42;
            });
            
            auto result2 = pool.submit_with_result(TaskPriority::HIGH, "高优先级任务", []() {
                return "Hello, World!";
            });
            
            std::cout << "Result 1: " << result1.get() << std::endl;
            std::cout << "Result 2: " << result2.get() << std::endl;
        }
        
        // 2. 优先级任务
        {
            std::cout << "\n=== 优先级任务示例 ===" << std::endl;
            ThreadPoolConfig config;
            config.min_threads = 2;
            config.max_threads = 4;
            ThreadPool pool(config);
            
            std::vector<std::future<void>> futures;
            
            // 提交不同优先级的任务
            for (int i = 0; i < 10; i++) {
                auto priority = static_cast<TaskPriority>(i % 5);
                futures.push_back(pool.submit_with_result(priority, 
                    "任务" + std::to_string(i), [i, priority]() {
                        std::ostringstream oss;
                        oss << "任务" << i << " 优先级" << static_cast<int>(priority)
                            << " 在" << std::this_thread::get_id() << "执行";
                        std::cout << oss.str() << std::endl;
                    }));
            }
            
            // 等待所有任务完成
            for (auto& fut : futures) {
                fut.wait();
            }
        }
        
        // 3. 带取消的任务
        {
            std::cout << "\n=== 可取消任务示例 ===" << std::endl;
            ThreadPool pool;
            
            auto [future, token] = pool.submit_with_cancellation(
                TaskPriority::NORMAL, "可取消任务", []() {
                    for (int i = 0; i < 10; i++) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        std::cout << "任务运行中: " << i << std::endl;
                    }
                    return 100;
                });
            
            // 1秒后取消任务
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            token->cancel();
            
            try {
                auto result = future.get();
                std::cout << "任务结果: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "任务异常: " << e.what() << std::endl;
            }
        }
        
        // 4. 定时任务
        {
            std::cout << "\n=== 定时任务示例 ===" << std::endl;
            ThreadPool pool;
            
            auto start = std::chrono::steady_clock::now();
            
            pool.submit_after(TaskPriority::NORMAL, "延迟任务",
                std::chrono::milliseconds(500), []() {
                    std::cout << "延迟500ms执行" << std::endl;
                });
            
            pool.submit_at(TaskPriority::NORMAL, "定时任务",
                start + std::chrono::seconds(1), []() {
                    std::cout << "1秒后执行" << std::endl;
                });
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
        // 5. 线程池管理器
        {
            std::cout << "\n=== 线程池管理器示例 ===" << std::endl;
            
            // 获取不同的线程池
            auto& io_pool = get_thread_pool("io");
            auto& compute_pool = get_thread_pool("compute");
            
            // 提交IO密集型任务
            auto io_future = io_pool.submit([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return "IO任务完成";
            });
            
            // 提交计算密集型任务
            auto compute_future = compute_pool.submit([]() {
                // 模拟计算
                int sum = 0;
                for (int i = 0; i < 1000000; i++) {
                    sum += i;
                }
                return sum;
            });
            
            std::cout << io_future.get() << std::endl;
            std::cout << "计算结果: " << compute_future.get() << std::endl;
            
            // 显示线程池状态
            std::cout << compute_pool.get_status_string() << std::endl;
        }
        
        // 6. 性能测试
        {
            std::cout << "\n=== 性能测试示例 ===" << std::endl;
            
            ThreadPoolConfig config;
            config.min_threads = 4;
            config.max_threads = 8;
            config.enable_work_stealing = true;
            config.enable_statistics = true;
            
            ThreadPool pool(config);
            
            constexpr int TASK_COUNT = 1000;
            std::vector<std::future<int>> futures;
            
            auto start = std::chrono::steady_clock::now();
            
            for (int i = 0; i < TASK_COUNT; i++) {
                futures.push_back(pool.submit([i]() {
                    // 模拟工作负载
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return i * i;
                }));
            }
            
            int total = 0;
            for (auto& fut : futures) {
                total += fut.get();
            }
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double>(end - start).count();
            
            std::cout << "完成 " << TASK_COUNT << " 个任务" << std::endl;
            std::cout << "总时间: " << duration << " 秒" << std::endl;
            std::cout << "平均每个任务: " << duration / TASK_COUNT * 1000 << " 毫秒" << std::endl;
            std::cout << "总结果: " << total << std::endl;
            
            // 显示统计信息
            auto stats = pool.get_stats();
            std::cout << "平均吞吐量: " << stats.tasks_per_second() << " 任务/秒" << std::endl;
        }
        
        // 7. 异常处理
        {
            std::cout << "\n=== 异常处理示例 ===" << std::endl;
            ThreadPool pool;
            
            try {
                auto future = pool.submit([]() {
                    throw std::runtime_error("任务执行异常");
                    return 42;
                });
                
                try {
                    auto result = future.get();
                } catch (const std::exception& e) {
                    std::cout << "捕获到异常: " << e.what() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "提交任务异常: " << e.what() << std::endl;
            }
        }
        
        // 关闭所有线程池
        ThreadPoolManager::get_instance().shutdown_all();
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n所有测试完成!" << std::endl;
    return 0;
}
