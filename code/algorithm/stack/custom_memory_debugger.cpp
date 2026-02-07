// custom_memory_debugger.cpp
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

class MemoryDebugger {
private:
    struct AllocationInfo {
        void* ptr;
        size_t size;
        void* callstack[20];
        int frames;
        const char* file;
        int line;
        
        AllocationInfo(void* p, size_t s, const char* f, int l) 
            : ptr(p), size(s), file(f), line(l) {
            // 获取调用栈
            frames = backtrace(callstack, 20);
        }
    };
    
    static std::map<void*, AllocationInfo> allocations;
    static size_t total_allocated;
    static size_t total_freed;
    static std::mutex mutex;
    
    static void print_stacktrace(void* const* callstack, int frames) {
        char** symbols = backtrace_symbols(callstack, frames);
        if (!symbols) return;
        
        for (int i = 0; i < frames; ++i) {
            Dl_info info;
            if (dladdr(callstack[i], &info) && info.dli_sname) {
                int status;
                char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    std::cerr << "    #" << i << " " << demangled 
                              << " in " << info.dli_fname << std::endl;
                    free(demangled);
                } else {
                    std::cerr << "    #" << i << " " << info.dli_sname 
                              << " in " << info.dli_fname << std::endl;
                }
            } else {
                std::cerr << "    #" << i << " " << symbols[i] << std::endl;
            }
        }
        free(symbols);
    }
    
public:
    static void* track_malloc(size_t size, const char* file, int line) {
        std::lock_guard<std::mutex> lock(mutex);
        
        void* ptr = malloc(size);
        if (ptr) {
            allocations[ptr] = AllocationInfo(ptr, size, file, line);
            total_allocated += size;
        }
        
        std::cerr << "[DEBUG] malloc(" << size << ") at " << file 
                  << ":" << line << " -> " << ptr << std::endl;
        return ptr;
    }
    
    static void track_free(void* ptr, const char* file, int line) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!ptr) return;
        
        auto it = allocations.find(ptr);
        if (it != allocations.end()) {
            total_freed += it->second.size;
            allocations.erase(it);
            std::cerr << "[DEBUG] free(" << ptr << ") at " << file 
                      << ":" << line << std::endl;
        } else {
            std::cerr << "[ERROR] 释放未分配的内存: " << ptr 
                      << " at " << file << ":" << line << std::endl;
            print_stacktrace(nullptr, 0);
        }
        
        free(ptr);
    }
    
    static void* track_new(size_t size, const char* file, int line) {
        return track_malloc(size, file, line);
    }
    
    static void track_delete(void* ptr, const char* file, int line) {
        track_free(ptr, file, line);
    }
    
    static void report() {
        std::lock_guard<std::mutex> lock(mutex);
        
        std::cerr << "\n=== 内存调试报告 ===" << std::endl;
        std::cerr << "总分配: " << total_allocated << " 字节" << std::endl;
        std::cerr << "总释放: " << total_freed << " 字节" << std::endl;
        std::cerr << "当前分配: " << (total_allocated - total_freed) << " 字节" << std::endl;
        std::cerr << "泄漏块数: " << allocations.size() << std::endl;
        
        if (!allocations.empty()) {
            std::cerr << "\n=== 内存泄漏详情 ===" << std::endl;
            for (const auto& pair : allocations) {
                const AllocationInfo& info = pair.second;
                std::cerr << "\n泄漏 " << info.size << " 字节在 " << info.file 
                          << ":" << info.line << std::endl;
                std::cerr << "指针: " << info.ptr << std::endl;
                std::cerr << "调用栈:" << std::endl;
                print_stacktrace(info.callstack, info.frames);
            }
        }
        
        std::cerr << "====================" << std::endl;
    }
    
    static void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        allocations.clear();
        total_allocated = 0;
        total_freed = 0;
    }
};

// 初始化静态成员
std::map<void*, MemoryDebugger::AllocationInfo> MemoryDebugger::allocations;
size_t MemoryDebugger::total_allocated = 0;
size_t MemoryDebugger::total_freed = 0;
std::mutex MemoryDebugger::mutex;

// 重载全局 new/delete
void* operator new(size_t size) {
    return MemoryDebugger::track_new(size, __FILE__, __LINE__);
}

void* operator new[](size_t size) {
    return MemoryDebugger::track_new(size, __FILE__, __LINE__);
}

void operator delete(void* ptr) noexcept {
    MemoryDebugger::track_delete(ptr, __FILE__, __LINE__);
}

void operator delete[](void* ptr) noexcept {
    MemoryDebugger::track_delete(ptr, __FILE__, __LINE__);
}

void operator delete(void* ptr, size_t) noexcept {
    MemoryDebugger::track_delete(ptr, __FILE__, __LINE__);
}

void operator delete[](void* ptr, size_t) noexcept {
    MemoryDebugger::track_delete(ptr, __FILE__, __LINE__);
}

// 测试程序
class TestClass {
public:
    int data[100];
    TestClass() { std::cout << "TestClass 构造" << std::endl; }
    ~TestClass() { std::cout << "TestClass 析构" << std::endl; }
};

void test_leak() {
    std::cout << "=== 测试内存泄漏 ===" << std::endl;
    
    // 1. 普通泄漏
    int* leak1 = new int(42);
    std::cout << "泄漏1: " << *leak1 << std::endl;
    
    // 2. 数组泄漏
    int* leak2 = new int[100];
    leak2[0] = 100;
    
    // 3. 正确释放
    int* ok = new int(999);
    std::cout << "正确: " << *ok << std::endl;
    delete ok;
    
    // 4. 对象泄漏
    TestClass* obj = new TestClass();
    // 忘记 delete obj
}

int main() {
    atexit(MemoryDebugger::report);
    
    test_leak();
    
    std::cout << "\n程序结束，查看内存报告..." << std::endl;
    return 0;
}
