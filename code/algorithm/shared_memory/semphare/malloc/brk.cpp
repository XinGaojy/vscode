// brk_sbrk_demo.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>

class BrkAllocator {
private:
    // 当前堆顶指针
    static void* program_break;
    
public:
    static void* allocate(size_t size) {
        // 获取当前堆顶
        void* old_break = sbrk(0);
        if (old_break == (void*)-1) {
            std::cerr << "sbrk(0) 失败" << std::endl;
            return nullptr;
        }
        
        // 向上调整堆顶
        if (brk((char*)old_break + size) != 0) {
            std::cerr << "brk 失败" << std::endl;
            return nullptr;
        }
        
        // 记录新的堆顶
        program_break = (char*)old_break + size;
        return old_break;
    }
    
    static void deallocate(void* ptr, size_t size) {
        // brk通常不支持部分释放，但可以调整堆顶
        // 注意：只能释放最近分配的内存
        if (ptr && (char*)ptr + size == program_break) {
            if (brk(ptr) == 0) {
                program_break = ptr;
            }
        }
    }
    
    static void print_break_info() {
        std::cout << "当前堆顶: " << sbrk(0) << std::endl;
    }
};

void* BrkAllocator::program_break = nullptr;

void demonstrate_brk_sbrk() {
    std::cout << "=== brk/sbrk 演示 ===" << std::endl;
    
    // 获取初始堆顶
    void* initial_break = sbrk(0);
    std::cout << "初始堆顶: " << initial_break << std::endl;
    
    // 分配内存
    const size_t size = 1024;  // 1KB
    void* ptr1 = BrkAllocator::allocate(size);
    std::cout << "分配 " << size << " 字节在: " << ptr1 << std::endl;
    BrkAllocator::print_break_info();
    
    // 使用内存
    int* arr = static_cast<int*>(ptr1);
    for (int i = 0; i < 10; ++i) {
        arr[i] = i * i;
    }
    std::cout << "数据: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
    
    // 再分配一些内存
    void* ptr2 = BrkAllocator::allocate(512);
    std::cout << "再分配 512 字节在: " << ptr2 << std::endl;
    BrkAllocator::print_break_info();
    
    // 尝试释放（实际上很少用）
    std::cout << "\n注意: brk/sbrk 通常用于分配，释放由程序结束自动处理" << std::endl;
    
    // 最后的堆顶
    void* final_break = sbrk(0);
    std::cout << "最终堆顶: " << final_break << std::endl;
    std::cout << "总分配: " 
              << (char*)final_break - (char*)initial_break 
              << " 字节" << std::endl;
}

int main() {
    demonstrate_brk_sbrk();
    return 0;
}
