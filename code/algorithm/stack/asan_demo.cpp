// asan_demo.cpp
#include <iostream>
#include <cstdlib>
#include <cstring>

// 1. 堆缓冲区溢出
void heap_buffer_overflow() {
    int* buffer = new int[10];
    
    // 写入越界
    for (int i = 0; i <= 10; ++i) {  // 错误：应该是 i < 10
        buffer[i] = i;
    }
    
    // 读取越界
    std::cout << "越界读取: " << buffer[10] << std::endl;
    
    delete[] buffer;
}

// 2. 使用已释放内存
void use_after_free() {
    int* ptr = new int(42);
    delete ptr;
    
    // 使用已释放的内存
    *ptr = 100;  // 错误
    std::cout << "使用已释放内存: " << *ptr << std::endl;
}

// 3. 双重释放
void double_free() {
    int* ptr = new int(42);
    delete ptr;
    delete ptr;  // 错误：双重释放
}

// 4. 内存泄漏
void memory_leak() {
    int* ptr = new int(42);
    // 忘记 delete
    std::cout << "内存泄漏: " << *ptr << std::endl;
}

int main() {
    std::cout << "=== AddressSanitizer 演示 ===" << std::endl;
    
    // heap_buffer_overflow();
    // use_after_free();
    // double_free();
    memory_leak();
    
    return 0;
}
