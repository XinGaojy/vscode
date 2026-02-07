// buggy_app.cpp
#include <iostream>
#include <vector>
#include <memory>
#include<sys/resource.h>
class MyClass {
public:
    MyClass(int id) : id(id), data(new int[100]) {
        std::cout << "构造 MyClass " << id << std::endl;
    }
    
    ~MyClass() {
        std::cout << "析构 MyClass " << id << std::endl;
        delete[] data;
    }
    
    void process() {
        std::cout << "处理 " << id << std::endl;
        // 模拟处理
        for (int i = 0; i <= 100; ++i) {  // 错误：越界
            data[i] = i * 10;
        }
    }
    
private:
    int id;
    int* data;
};

void process_objects() {
    std::vector<std::shared_ptr<MyClass>> objects;
    
    // 创建对象
    for (int i = 0; i < 5; ++i) {
        objects.push_back(std::make_shared<MyClass>(i));
    }
    
    // 处理对象
    for (auto& obj : objects) {
        obj->process();
    }
    
    // 双重释放
    if (!objects.empty()) {
        objects[0].reset();  // 释放
        // 隐式再次释放
    }
}

void recursive_func(int depth) {
    if (depth <= 0) return;
    
    char buffer[1024];  // 栈分配
    buffer[depth] = 'A';  // 写入
    
    // 递归调用
    recursive_func(depth - 1);
    
    // 读取
    std::cout << "深度 " << depth << ": " << buffer[depth] << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "程序开始" << std::endl;
    
    // 启用 core dump
    struct rlimit core_limit = {RLIM_INFINITY, RLIM_INFINITY};
    setrlimit(RLIMIT_CORE, &core_limit);
    
    int choice = 1;
    if (argc > 1) {
        choice = std::atoi(argv[1]);
    }
    
    switch (choice) {
        case 1:
            std::cout << "测试1: 堆损坏" << std::endl;
            process_objects();
            break;
            
        case 2:
            std::cout << "测试2: 栈溢出" << std::endl;
            recursive_func(10000);
            break;
            
        case 3: {
            std::cout << "测试3: 段错误" << std::endl;
            int* ptr = nullptr;
            *ptr = 42;  // 段错误
            break;
        }
            
        default:
            std::cout << "无效测试" << std::endl;
    }
    
    return 0;
}
