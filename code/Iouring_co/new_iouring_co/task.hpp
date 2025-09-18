#pragma once
#include <coroutine>
#include <functional>
#include <exception>
struct Task {
    struct promise_type {
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
    bool done() const { return !handle_ || handle_.done(); }

private:
    handle_t handle_;
};