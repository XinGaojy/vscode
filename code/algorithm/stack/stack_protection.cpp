// stack_protection.cpp
#include <iostream>
#include <cstring>
#include <csetjmp>
#include<unistd.h>
#include<stdlib.h>
// 1. 使用 canary 检测栈溢出
class StackCanary {
private:
    static const unsigned long CANARY = 0xDEADBEEFCAFEBABE;
    unsigned long canary;
    
public:
    StackCanary() : canary(CANARY) {}
    
    ~StackCanary() {
        if (canary != CANARY) {
            std::cerr << "栈溢出检测到！canary 被修改: 0x" 
                      << std::hex << canary << std::dec << std::endl;
            std::exit(1);
        }
    }
};

void test_with_canary() {
    StackCanary canary;  // 在栈帧开始处
    
    char buffer[10];
    std::cout << "缓冲区地址: " << (void*)buffer << std::endl;
    std::cout << "canary 地址: " << (void*)&canary << std::endl;
    
    // 故意溢出
    std::strcpy(buffer, "这是一个很长的字符串，会溢出缓冲区");
    
    std::cout << "缓冲区内容: " << buffer << std::endl;
    // canary 的析构函数会检测到溢出
}

// 2. 使用 setjmp/longjmp 处理栈溢出
jmp_buf overflow_env;

void handle_overflow(int sig) {
    std::cerr << "\n捕获栈溢出信号: " << sig << std::endl;
    longjmp(overflow_env, 1);
}

void test_overflow_detection() {
    // 设置信号处理器
    signal(SIGSEGV, handle_overflow);
    signal(SIGBUS, handle_overflow);
    
    if (setjmp(overflow_env) == 0) {
        // 正常执行
        char buffer[10];
        std::cout << "测试缓冲区溢出..." << std::endl;
        
        // 这会导致栈溢出
        for (int i = 0; i < 1000; ++i) {
            buffer[i] = 'A';
        }
        
        std::cout << "正常完成" << std::endl;
    } else {
        std::cerr << "从栈溢出恢复" << std::endl;
    }
}

// 3. 栈使用量监控
#include <sys/resource.h>
#include <sys/time.h>

void monitor_stack_usage() {
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    std::cout << "最大栈使用量: " << usage.ru_maxrss << " KB" << std::endl;
    std::cout << "软页错误: " << usage.ru_minflt << std::endl;
    std::cout << "硬页错误: " << usage.ru_majflt << std::endl;
}

void recursive_function(int depth) {
    char buffer[1024];
    std::memset(buffer, 'A', sizeof(buffer));
    
    if (depth % 100 == 0) {
        monitor_stack_usage();
    }
    
    if (depth < 1000) {
        recursive_function(depth + 1);
    }
}

int main() {
    std::cout << "=== 栈保护机制演示 ===" << std::endl;
    
    std::cout << "\n1. 使用 canary 检测栈溢出：" << std::endl;
    test_with_canary();
    
    std::cout << "\n2. 使用信号处理栈溢出：" << std::endl;
    test_overflow_detection();
    
    std::cout << "\n3. 监控栈使用量：" << std::endl;
    recursive_function(0);
    
    return 0;
}
