#include <iostream>
#include <utility>
#include <functional>
#include <memory>
using namespace std;
template <typename T, typename Deleter = std::default_delete<T>>
class Unique_ptr {
public:
    // 构造函数
    Unique_ptr(T* ptr = nullptr, Deleter deleter = Deleter()) 
        : ptr_(ptr), deleter_(deleter) {}

    // 禁止拷贝构造和赋值
    Unique_ptr(const Unique_ptr&) = delete;
    Unique_ptr& operator=(const Unique_ptr&) = delete;

    // 支持移动构造和赋值
    Unique_ptr(Unique_ptr&& other) noexcept
        : ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
        other.ptr_ = nullptr;
    }

    Unique_ptr& operator=(Unique_ptr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            deleter_ = std::move(other.deleter_);
        }
        return *this;
    }

    // 重置指针
    void reset(T* new_ptr = nullptr) {
        if (ptr_ != new_ptr) {
            deleter_(ptr_);
            ptr_ = new_ptr;
        }
    }

    // 释放指针
    T* release() {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

    // 获取指针
    T* get() const { return ptr_; }

    // 解引用操作符
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    // 析构函数
    ~Unique_ptr() {
        reset();
    }

private:
    T* ptr_;
    Deleter deleter_;
};

// 自定义删除器示例
struct custom_deleter {
    void operator()(int* ptr) {
        std::cout << "Custom deleter called" << std::endl;
        delete ptr;
    }
};

int main() { 
    // 使用默认删除器
    Unique_ptr<int> ptr1(new int(42));
    std::cout << *ptr1 << std::endl;

    // 使用自定义删除器
    Unique_ptr<int, custom_deleter> ptr2(new int(100), custom_deleter());
    std::cout << *ptr2 << std::endl;

    // 移动语义
    Unique_ptr<int> ptr3(std::move(ptr1));
    std::cout << *ptr3 << std::endl;

    return 0;
}