#include <iostream>
#include <cstdlib>
#define DEBUG_MEMORY 1
// 简单内存跟踪
#ifdef DEBUG_MEMORY
#include <map>
#include <string>
#include <cstring>

class MemoryTracker {
private:
    struct AllocationInfo {
        void* ptr;
        size_t size;
        const char* file;
        int line;
    };
    
    static std::map<void*, AllocationInfo> allocations;
    static size_t total_allocated;
    static size_t total_freed;
    
public:
    static void* track_allocation(size_t size, const char* file, int line) {
        void* ptr = std::malloc(size);
        if (ptr) {
            allocations[ptr] = {ptr, size, file, line};
            total_allocated += size;
        }
        return ptr;
    }
    
    static void track_free(void* ptr) {
        auto it = allocations.find(ptr);
        if (it != allocations.end()) {
            total_freed += it->second.size;
            allocations.erase(it);
        }
        std::free(ptr);
    }
    
    static void report_leaks() {
        if (!allocations.empty()) {
            std::cout << "\n=== 内存泄漏报告 ===" << std::endl;
            std::cout << "总分配: " << total_allocated << " 字节" << std::endl;
            std::cout << "总释放: " << total_freed << " 字节" << std::endl;
            std::cout << "泄漏: " << allocations.size() << " 块, " 
                      << (total_allocated - total_freed) << " 字节" << std::endl;
            
            for (const auto& pair : allocations) {
                const auto& info = pair.second;
                std::cout << "泄漏: " << info.ptr << " (" << info.size 
                          << " 字节) 在 " << info.file 
                          << ":" << info.line << std::endl;
            }
        } else {
            std::cout << "无内存泄漏" << std::endl;
        }
    }
};

std::map<void*, MemoryTracker::AllocationInfo> MemoryTracker::allocations;
size_t MemoryTracker::total_allocated = 0;
size_t MemoryTracker::total_freed = 0;

// 重载 new/delete
void* operator new(size_t size, const char* file, int line) {
    return MemoryTracker::track_allocation(size, file, line);
}

void* operator new[](size_t size, const char* file, int line) {
    return MemoryTracker::track_allocation(size, file, line);
}

void operator delete(void* ptr) noexcept {
    MemoryTracker::track_free(ptr);
}

void operator delete[](void* ptr) noexcept {
    MemoryTracker::track_free(ptr);
}

void operator delete(void* ptr, const char* file, int line) noexcept {
    MemoryTracker::track_free(ptr);
}

void operator delete[](void* ptr, const char* file, int line) noexcept {
    MemoryTracker::track_free(ptr);
}

#define new new(__FILE__, __LINE__)
#endif

void demonstrate_memory_debugging() {
    std::cout << "\n=== 内存调试和检测 ===" << std::endl;
    
    #ifdef DEBUG_MEMORY
    std::cout << "内存调试已启用" << std::endl;
    
    // 正常分配
    int* normal = new int(42);
    delete normal;
    
    // 泄漏
    int* leak1 = new int(100);
    int* leak2 = new int[10];
   
    // 清理
    delete leak1;
    delete[] leak2;
   
    // 报告泄漏
    MemoryTracker::report_leaks();
 

    #else
    std::cout << "内存调试未启用（定义 DEBUG_MEMORY 启用）" << std::endl;
    #endif
    
    // 使用工具检测内存问题
    std::cout << "\n推荐的内存调试工具：" << std::endl;
    std::cout << "1. Valgrind (Linux/Mac)" << std::endl;
    std::cout << "2. AddressSanitizer (GCC/Clang)" << std::endl;
    std::cout << "3. Dr. Memory (Windows)" << std::endl;
    std::cout << "4. Visual Studio 诊断工具" << std::endl;
}

int main(){
    
    demonstrate_memory_debugging();
    return 0;
}
