#include <iostream>
#include <utility>  // 用于 std::swap, std::move

template<typename T>
class my_unique_ptr {
private:
    T* ptr;  // 原始指针

public:
    // 1. 构造函数
    explicit my_unique_ptr(T* p = nullptr) : ptr(p) {}
    
    // 2. 禁用拷贝构造和拷贝赋值（独占所有权）
    my_unique_ptr(const my_unique_ptr&) = delete;
    my_unique_ptr& operator=(const my_unique_ptr&) = delete;
    
    // 3. 移动构造函数
    my_unique_ptr(my_unique_ptr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;  // 转移所有权
    }
    
    // 4. 移动赋值运算符
    my_unique_ptr& operator=(my_unique_ptr&& other) noexcept {
        if (this != &other) {
            delete ptr;        // 释放当前资源
            ptr = other.ptr;   // 获取新资源
            other.ptr = nullptr;  // 原指针置空
        }
        return *this;
    }
    
    // 5. 析构函数
    ~my_unique_ptr() {
        delete ptr;
    }
    
    // 6. 解引用运算符
    T& operator*() const {
        return *ptr;
    }
    
    // 7. 箭头运算符
    T* operator->() const {
        return ptr;
    }
    
    // 8. 获取原始指针
    T* get() const {
        return ptr;
    }
    
    // 9. 释放所有权
    T* release() {
        T* temp = ptr;
        ptr = nullptr;
        return temp;
    }
    
    // 10. 重置指针
    void reset(T* p = nullptr) {
        delete ptr;  // 释放原资源
        ptr = p;     // 指向新资源
    }
    
    // 11. 交换指针
    void swap(my_unique_ptr& other) {
        std::swap(ptr, other.ptr);
    }
    
    // 12. 布尔转换（检查是否为空）
    explicit operator bool() const {
        return ptr != nullptr;
    }
};

// 示例类
class MyClass {
public:
    int value;
    MyClass(int v) : value(v) {
        std::cout << "MyClass constructed with value: " << value << std::endl;
    }
    
    void print() const {
        std::cout << "MyClass value: " << value << std::endl;
    }
    
    ~MyClass() {
        std::cout << "MyClass destroyed, value was: " << value << std::endl;
    }
};

int main() {
    std::cout << "=== 测试 my_unique_ptr ===" << std::endl;
    
    // 1. 创建 unique_ptr
    my_unique_ptr<MyClass> ptr1(new MyClass(10));
    ptr1->print();
    
    // 2. 使用解引用运算符
    (*ptr1).value = 20;
    ptr1->print();
    
    // 3. 移动语义测试
    {
        my_unique_ptr<MyClass> ptr2 = std::move(ptr1);
        if (!ptr1) {
            std::cout << "ptr1 现在是空的" << std::endl;
        }
        ptr2->print();
        
        // ptr2 离开作用域会自动释放资源
    }
    
    // 4. 重置指针测试
    ptr1.reset(new MyClass(30));
    ptr1->print();
    
    // 5. 释放所有权测试
    MyClass* raw_ptr = ptr1.release();
    std::cout << "原始指针获取的值: " << raw_ptr->value << std::endl;
    delete raw_ptr;  // 需要手动释放
    
    // 6. 布尔转换测试
    if (!ptr1) {
        std::cout << "ptr1 是空的" << std::endl;
    }
    
    // 7. 使用 get() 方法
    ptr1.reset(new MyClass(40));
    MyClass* ptr = ptr1.get();
    std::cout << "通过 get() 获取的值: " << ptr->value << std::endl;
    
    return 0;
}














