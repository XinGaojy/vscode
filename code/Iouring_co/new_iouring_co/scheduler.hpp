#pragma once
#include "task.hpp"
#include "io_engine.hpp"
#include <vector>
#include <queue>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <iostream>

class Scheduler;

class Worker {
public:
    Worker(Scheduler* scheduler, bool try_uring);
    ~Worker();

    void run();
    void stop();
    void enqueue(Task task);
    bool steal_work(Task& out_task);
    IoEngine& engine() { return *engine_; }

private:
    void init_engine(bool try_uring);

    Scheduler* scheduler_;
    std::unique_ptr<IoEngine> engine_;
    std::queue<Task> ready_queue_;
    std::mutex queue_mutex_;
    std::atomic<bool> running_{false};
};

class Scheduler {
public:
    explicit Scheduler(size_t worker_count = std::thread::hardware_concurrency());
    ~Scheduler();

    void schedule(Task task);
    void run();
    void fallback_to_epoll();

    static thread_local Worker* current_worker;

private:
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<size_t> next_worker_{0};
    std::atomic<size_t> task_count_{0};
};