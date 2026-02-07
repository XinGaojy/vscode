// memory_leak_demo.cpp
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>

// 1. 简单内存泄漏
void simple_leak() {
    int* ptr = new int(42);
    // 忘记 delete
    std::cout << "简单泄漏: " << *ptr << std::endl;
    // 这里应该写 delete ptr;
}

// 2. 数组泄漏
void array_leak() {
    int* arr = new int[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }
    // 忘记 delete[]
    std::cout << "数组泄漏: " << arr[0] << std::endl;
    // 这里应该写 delete[] arr;
}

// 3. 循环引用导致智能指针泄漏
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // 使用 weak_ptr 避免循环引用
    int value;
    
    Node(int v) : value(v) {
        std::cout << "构造 Node " << v << std::endl;
    }
    
    ~Node() {
        std::cout << "析构 Node " << value << std::endl;
    }
};

void circular_reference_leak() {
    std::cout << "\n循环引用泄漏演示：" << std::endl;
    auto node1 = std::make_shared<Node>(1);
    auto node2 = std::make_shared<Node>(2);
    
    // 创建循环引用
    node1->next = node2;
    // node2->prev = node1;  // 应该用 weak_ptr
    
    std::cout << "node1 引用计数: " << node1.use_count() << std::endl;
    std::cout << "node2 引用计数: " << node2.use_count() << std::endl;
    
    // 离开作用域，node1 和 node2 应该被销毁
    // 但由于循环引用，它们都不会被销毁
}

// 4. 异常安全导致泄漏
void exception_leak() {
    int* resource1 = new int(100);
    int* resource2 = new int(200);
    
    try {
        // 可能抛出异常的操作
        if (std::rand() % 2 == 0) {
            throw std::runtime_error("随机异常");
        }
        
        // 正常流程
        std::cout << "资源1: " << *resource1 << std::endl;
        std::cout << "资源2: " << *resource2 << std::endl;
        
        delete resource1;
        delete resource2;
    } catch (...) {
        std::cout << "捕获异常" << std::endl;
        // 这里需要清理资源！
        // 应该添加：delete resource1; delete resource2;
        throw;
    }
}

// 5. 使用 RAII 避免泄漏
class ManagedResource {
private:
    int* ptr;
    
public:
    explicit ManagedResource(int value) : ptr(new int(value)) {
        std::cout << "分配资源: " << *ptr << std::endl;
    }
    
    ~ManagedResource() {
        std::cout << "释放资源: " << *ptr << std::endl;
        delete ptr;
    }
    
    int get() const { return *ptr; }
    void set(int value) { *ptr = value; }
    
    // 禁止拷贝
    ManagedResource(const ManagedResource&) = delete;
    ManagedResource& operator=(const ManagedResource&) = delete;
    
    // 允许移动
    ManagedResource(ManagedResource&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    ManagedResource& operator=(ManagedResource&& other) noexcept {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
};

void safe_with_raii() {
    ManagedResource r1(100);
    ManagedResource r2(200);
    
    std::cout << "RAII 安全: r1=" << r1.get() << ", r2=" << r2.get() << std::endl;
    
    // 即使抛出异常，资源也会被正确释放
    if (std::rand() % 3 == 0) {
        throw std::runtime_error("测试异常");
    }
}

int main() {
    std::cout << "=== 内存泄漏演示程序 ===" << std::endl;
    
    // 测试各种泄漏
    simple_leak();
    array_leak();
    circular_reference_leak();
    
    try {
        exception_leak();
    } catch (...) {
        std::cout << "异常泄漏测试完成" << std::endl;
    }
    
    try {
        safe_with_raii();
    } catch (...) {
        std::cout << "RAII 测试完成（即使有异常）" << std::endl;
    }
    
    std::cout << "\n程序结束，观察哪些内存泄漏了" << std::endl;
    return 0;
}
