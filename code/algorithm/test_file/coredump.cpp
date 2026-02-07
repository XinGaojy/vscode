// memory_issues.cpp
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>

// 1. 堆栈缓冲区溢出
void buffer_overflow_heap() {
    char* buffer = new char[10];
    strcpy(buffer, "This is a very long string that will overflow");
    delete[] buffer;
}

// 2. 栈缓冲区溢出
void buffer_overflow_stack() {
    char buffer[10];
    strcpy(buffer, "This is a very long string that will overflow");
}

// 3. 使用未初始化内存
void uninitialized_memory() {
    int* ptr = new int;  // 未初始化
    std::cout << "未初始化的值: " << *ptr << std::endl;
    delete ptr;
}

// 4. 内存泄漏
void memory_leak() {
    for (int i = 0; i < 1000; ++i) {
        int* leak = new int[1000];
        // 忘记delete
    }
}

// 5. 错误的释放
void bad_free() {
    int array[10];
    delete[] array;  // 错误：栈上数组用delete
    
    int* ptr = new int;
    free(ptr);  // 错误：new分配，用free释放
    
    int* arr = new int[10];
    delete arr;  // 错误：数组应该用delete[]
}

// 6. 多线程竞争条件
#include <thread>
#include <mutex>

int shared_counter = 0;
std::mutex mtx;

void increment_without_lock() {
    for (int i = 0; i < 1000000; ++i) {
        ++shared_counter;  // 数据竞争
    }
}

void data_race() {
    std::thread t1(increment_without_lock);
    std::thread t2(increment_without_lock);
    
    t1.join();
    t2.join();
    
    std::cout << "计数器: " << shared_counter 
              << " (应该是: 2000000)" << std::endl;
}

int main() {
    std::cout << "选择内存错误类型：" << std::endl;
    std::cout << "1. 堆缓冲区溢出" << std::endl;
    std::cout << "2. 栈缓冲区溢出" << std::endl;
    std::cout << "3. 使用未初始化内存" << std::endl;
    std::cout << "4. 内存泄漏" << std::endl;
    std::cout << "5. 错误的释放" << std::endl;
    std::cout << "6. 数据竞争" << std::endl;
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: buffer_overflow_heap(); break;
        case 2: buffer_overflow_stack(); break;
        case 3: uninitialized_memory(); break;
        case 4: memory_leak(); break;
        case 5: bad_free(); break;
        case 6: data_race(); break;
    }
    
    return 0;
}
