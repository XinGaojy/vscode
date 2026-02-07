//简单版本实现-----------------------------------------------------
#if 0
#include <cstdlib>  // malloc, free
#include <cstddef>  // size_t, ptrdiff_t
#include <new>      // bad_alloc, nothrow
#include <type_traits>
#include <iostream>

namespace my_new {

// 自定义 bad_alloc 异常
class bad_alloc : public std::bad_alloc {
public:
    bad_alloc() noexcept = default;
    const char* what() const noexcept override {
        return "my_new::bad_alloc: memory allocation failed";
    }
};

// 1. 简单的 new 实现
template<typename T, typename... Args>
T* simple_new(Args&&... args) {
    // 1. 分配内存
    void* memory = std::malloc(sizeof(T));
    if (!memory) {
        throw bad_alloc();  // 分配失败抛出异常
    }
    
    try {
        // 2. 构造对象
        T* ptr = new(memory) T(std::forward<Args>(args)...);
        return ptr;
    } catch (...) {
        // 3. 构造失败，释放内存
        std::free(memory);
        throw;  // 重新抛出异常
    }
}

// 2. 简单的 delete 实现
template<typename T>
void simple_delete(T* ptr) noexcept {
    if (ptr) {
        // 1. 调用析构函数
        ptr->~T();
        // 2. 释放内存
        std::free(ptr);
    }
}

};
// 测试基础功能
void test_basic() {
    std::cout << "=== 基础 new/delete 实现 ===" << std::endl;
    
    // 测试 POD 类型
    int* p1 = my_new::simple_new<int>(42);
    std::cout << "int: " << *p1 << std::endl;
    my_new::simple_delete(p1);
    
    // 测试类类型
    class MyClass {
    public:
        int x;
        double y;
        MyClass(int a, double b) : x(a), y(b) {
            std::cout << "构造 MyClass(" << a << ", " << b << ")" << std::endl;
        }
        ~MyClass() {
            std::cout << "析构 MyClass" << std::endl;
        }
    };
    
    MyClass* p2 = my_new::simple_new<MyClass>(10, 3.14);
    std::cout << "MyClass: (" << p2->x << ", " << p2->y << ")" << std::endl;
    my_new::simple_delete(p2);
}

int main(){

  test_basic();
  return 0;
}



#endif 

//完整实现
#if 1
namespace my_new {

// 3. 数组 new 实现
// 数组 new 需要存储元素数量，用于 delete[]
template<typename T>
T* array_new(std::size_t count) {
    if (count == 0) {
        return nullptr;
    }
    
    // 分配额外空间存储元素数量
    // 在数组前面存储一个 size_t 表示元素数量
    std::size_t total_size = sizeof(std::size_t) + sizeof(T) * count;
    void* raw_memory = std::malloc(total_size);
    
    if (!raw_memory) {
        throw bad_alloc();
    }
    
    // 存储元素数量
    std::size_t* count_ptr = static_cast<std::size_t*>(raw_memory);
    *count_ptr = count;
    
    // 计算数组的起始位置
    T* array_start = reinterpret_cast<T*>(count_ptr + 1);
    
    // 构造每个元素
    std::size_t constructed = 0;
    try {
        for (std::size_t i = 0; i < count; ++i) {
            new(&array_start[i]) T();  // 默认构造
            ++constructed;
        }
    } catch (...) {
        // 如果构造失败，销毁已构造的元素
        for (std::size_t i = 0; i < constructed; ++i) {
            array_start[i].~T();
        }
        std::free(raw_memory);
        throw;
    }
    
    return array_start;
}

// 4. 数组 delete 实现
template<typename T>
void array_delete(T* ptr) noexcept {
    if (!ptr) return;
    
    // 获取元素数量
    std::size_t* count_ptr = reinterpret_cast<std::size_t*>(ptr) - 1;
    std::size_t count = *count_ptr;
    
    // 反向调用析构函数
    for (std::size_t i = count; i > 0; --i) {
        ptr[i - 1].~T();
    }
    
    // 释放内存
    std::free(count_ptr);
}

// 5. 不抛出异常的 new
template<typename T, typename... Args>
T* nothrow_new(Args&&... args) noexcept {
    void* memory = std::malloc(sizeof(T));
    if (!memory) {
        return nullptr;
    }
    
    try {
        T* ptr = new(memory) T(std::forward<Args>(args)...);
        return ptr;
    } catch (...) {
        std::free(memory);
        return nullptr;
    }
}

// 6. 对齐分配
template<typename T, typename... Args>
T* aligned_new(std::size_t alignment, Args&&... args) {
    // 检查对齐值是否有效
    if ((alignment & (alignment - 1)) != 0) {  // 不是2的幂
        throw std::bad_alloc();
    }
    
    // 计算总大小：对象大小 + 对齐要求 - 1
    std::size_t total_size = sizeof(T) + alignment - 1;
    
    // 分配原始内存
    void* raw_memory = std::malloc(total_size + sizeof(void*));
    if (!raw_memory) {
        throw bad_alloc();
    }
    
    // 对齐内存
    char* aligned_ptr = reinterpret_cast<char*>(raw_memory) + sizeof(void*);
    std::size_t offset = alignment - (reinterpret_cast<std::uintptr_t>(aligned_ptr) & (alignment - 1));
    
    if (offset == alignment) {
        offset = 0;
    }
    
    char* aligned_memory = aligned_ptr + offset;
    
    // 存储原始指针
    void** original_ptr = reinterpret_cast<void**>(aligned_memory) - 1;
    *original_ptr = raw_memory;
    
    try {
        T* ptr = new(aligned_memory) T(std::forward<Args>(args)...);
        return ptr;
    } catch (...) {
        std::free(raw_memory);
        throw;
    }
}

// 7. 对齐释放
template<typename T>
void aligned_delete(T* ptr) noexcept {
    if (!ptr) return;
    
    // 获取原始指针
    void* raw_memory = *(reinterpret_cast<void**>(ptr) - 1);
    
    // 调用析构函数
    ptr->~T();
    
    // 释放内存
    std::free(raw_memory);
}

// 8. 定位 new（placement new）
template<typename T, typename... Args>
T* placement_new(void* memory, Args&&... args) noexcept {
    // 不分配内存，只在指定位置构造对象
    return new(memory) T(std::forward<Args>(args)...);
}

// 9. 定位 delete
template<typename T>
void placement_delete(T* ptr) noexcept {
    if (ptr) {
        ptr->~T();
    }
    // 注意：不释放内存！
}

};
// 测试完整功能
void test_complete() {
    std::cout << "\n=== 完整 new/delete 实现测试 ===" << std::endl;
    
    // 1. 测试数组 new/delete
    std::cout << "\n1. 数组 new/delete：" << std::endl;
    {
        class Tracked {
        public:
            static int count;
            int id;
            
            Tracked() : id(++count) {
                std::cout << "构造 Tracked #" << id << std::endl;
            }
            
            ~Tracked() {
                std::cout << "析构 Tracked #" << id << std::endl;
            }
        };
        int Tracked::count = 0;
        
        Tracked* arr = array_new<Tracked>(5);
        std::cout << "数组大小: 5" << std::endl;
        array_delete(arr);
    }
    
    // 2. 测试 nothrow new
    std::cout << "\n2. nothrow new：" << std::endl;
    {
        int* p = nothrow_new<int>(999);
        if (p) {
            std::cout << "分配成功: " << *p << std::endl;
            simple_delete(p);
        } else {
            std::cout << "分配失败" << std::endl;
        }
    }
    
    // 3. 测试对齐 new
    std::cout << "\n3. 对齐 new（64字节对齐）：" << std::endl;
    {
        struct alignas(64) AlignedStruct {
            char data[64];
            int value;
            
            AlignedStruct(int v) : value(v) {
                std::cout << "构造 AlignedStruct，地址: " << this 
                          << "，对齐: " << (reinterpret_cast<std::uintptr_t>(this) % 64) 
                          << std::endl;
            }
            
            ~AlignedStruct() {
                std::cout << "析构 AlignedStruct" << std::endl;
            }
        };
        
        AlignedStruct* aligned = aligned_new<AlignedStruct>(64, 123);
        if (aligned) {
            std::cout << "对齐分配的值: " << aligned->value << std::endl;
            aligned_delete(aligned);
        }
    }
    
    // 4. 测试 placement new
    std::cout << "\n4. placement new：" << std::endl;
    {
        char buffer[sizeof(std::string)];
        std::cout << "缓冲区地址: " << static_cast<void*>(buffer) << std::endl;
        
        std::string* str = placement_new<std::string>(buffer, "Hello Placement New!");
        std::cout << "placement new 字符串: " << *str << std::endl;
        
        placement_delete(str);
    }
}

int main(){
    
    test_complete();
    return 0;
}

#endif


