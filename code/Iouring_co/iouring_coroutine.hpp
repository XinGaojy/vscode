#pragma once
#include <coroutine>
#include <functional>

class IoUringAwaiter;

// 协程Promise类型
class CoroutinePromise {
public:
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
    std::coroutine_handle<> get_return_object() { 
        return std::coroutine_handle<>::from_promise(*this); 
    }
};

// 协程句柄
class Coroutine : public std::coroutine_handle<CoroutinePromise> {
public:
    using promise_type = CoroutinePromise;
};

// IO等待器（支持io_uring操作）
class IoUringAwaiter {
public:
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) {
        m_coro = h;
        submit_io(); // 提交io_uring请求
    }
    int await_resume() noexcept { return m_result; }

protected:
    virtual void submit_io() = 0;
    std::coroutine_handle<> m_coro;
    int m_result = 0;
};