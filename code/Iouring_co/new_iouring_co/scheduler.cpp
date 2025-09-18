#include "scheduler.hpp"
#include <unistd.h>
#include <sys/syscall.h>

thread_local Worker* Scheduler::current_worker = nullptr;

Worker::Worker(Scheduler* scheduler, bool try_uring) : scheduler_(scheduler) {
    init_engine(try_uring);
}

void Worker::init_engine(bool try_uring) {
#ifdef HAVE_LIBURING
    if (try_uring) {
        try {
            engine_ = std::make_unique<IoUringEngine>();
            return;
        } catch (...) {
            std::cerr << "Fallback to epoll" << std::endl;
        }
    }
#endif
    engine_ = std::make_unique<EpollEngine>();
}

void Worker::run() {
    running_ = true;
    Scheduler::current_worker = this;

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
            --scheduler_->task_count_;
        }

        // 工作窃取
        if (local_queue.empty() && running_) {
            Task stolen_task;
            for (auto& w : scheduler_->workers_) {
                if (w.get() != this && w->steal_work(stolen_task)) {
                    stolen_task.resume();
                    --scheduler_->task_count_;
                    break;
                }
            }
        }

        // 无任务时休眠
        if (scheduler_->task_count_ == 0 && running_) {
#ifdef HAVE_LIBURING
            if (auto* uring = dynamic_cast<IoUringEngine*>(engine_.get())) {
                __io_uring_sqring_wait(&uring->ring_);
            } else 
#endif
            {
                usleep(1000); // 1ms休眠
            }
        }
    }
}

void Worker::stop() {
    running_ = false;
    engine_->stop();
}

void Worker::enqueue(Task task) {
    std::lock_guard lock(queue_mutex_);
    ready_queue_.push(std::move(task));
}

bool Worker::steal_work(Task& out_task) {
    std::lock_guard lock(queue_mutex_);
    if (!ready_queue_.empty()) {
        out_task = std::move(ready_queue_.front());
        ready_queue_.pop();
        return true;
    }
    return false;
}

Scheduler::Scheduler(size_t worker_count) {
    for (size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back(std::make_unique<Worker>(this, true));
    }
}

Scheduler::~Scheduler() {
    for (auto& w : workers_) w->stop();
}

void Scheduler::schedule(Task task) {
    ++task_count_;
    size_t idx = next_worker_.fetch_add(1, std::memory_order_relaxed) % workers_.size();
    workers_[idx]->enqueue(std::move(task));
}

void Scheduler::run() {
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

void Scheduler::fallback_to_epoll() {
    for (auto& w : workers_) {
        w->stop();
        w->init_engine(false);
    }
}