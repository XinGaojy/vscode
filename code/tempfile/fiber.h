#ifndef FLARE_FIBER_H
#define FLARE_FIBER_H

#include <ucontext.h>
#include <functional>
#include <memory>
#include <iostream>

namespace flare {

class Fiber {
public:
    using Func = std::function<void()>;
    
    // 构造函数：创建纤程并分配栈空间
    explicit Fiber(Func&& func, size_t stack_size = 64 * 1024)
        : _func(std::move(func)) {
        _stack = new char[stack_size];
        init_context(stack_size);
    }
    
    ~Fiber() {
        delete[] _stack; // 释放纤程栈
    }
    
    // 切换到当前纤程执行
    void resume() {
        if (_finished) return;
        
        _caller = current();  // 保存调用者
        current() = this;     // 设置当前纤程
        swapcontext(&_caller->_ctx, &_ctx); // 上下文切换
    }
    
    // 从当前纤程切换回调用者
    static void yield() {
        auto self = current();
        current() = self->_caller; // 恢复调用者
        swapcontext(&self->_ctx, &self->_caller->_ctx);
    }
    
    bool finished() const { return _finished; }
    
    // 获取当前正在执行的纤程
    static Fiber*& current() {
        static thread_local Fiber* _current = nullptr;
        return _current;
    }

private:
    // 初始化ucontext_t上下文
    void init_context(size_t stack_size) {
        getcontext(&_ctx);
        _ctx.uc_stack.ss_sp = _stack;
        _ctx.uc_stack.ss_size = stack_size;
        _ctx.uc_link = nullptr; // 执行完后不自动跳转
        
        // 设置入口函数
        makecontext(&_ctx, []() {
            auto self = Fiber::current();
            try {
                self->_func();  // 执行用户函数
            } catch (...) {
                std::cerr << "Fiber terminated with exception" << std::endl;
            }
            self->_finished = true;
            Fiber::yield();     // 执行完成后自动yield
        }, 0);
    }

    ucontext_t _ctx;      // 上下文结构
    char* _stack;         // 纤程栈空间
    Func _func;           // 用户函数
    bool _finished = false; // 完成标志
    Fiber* _caller = nullptr; // 调用者纤程
};

} // namespace flare

#endif // FLARE_FIBER_H
