// mmap_demo.cpp
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

class MMapAllocator {
public:
    // 分配匿名内存（不关联文件）
    static void* allocate_anonymous(size_t size, int prot = PROT_READ | PROT_WRITE,
                                    int flags = MAP_PRIVATE | MAP_ANONYMOUS) {
        void* ptr = mmap(nullptr, size, prot, flags, -1, 0);
        if (ptr == MAP_FAILED) {
            perror("mmap 失败");
            return nullptr;
        }
        return ptr;
    }
    
    // 映射文件到内存
    static void* map_file(const char* filename, size_t* length = nullptr) {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("打开文件失败");
            return nullptr;
        }
        
        // 获取文件大小
        struct stat st;
        if (fstat(fd, &st) == -1) {
            perror("获取文件状态失败");
            close(fd);
            return nullptr;
        }
        
        size_t file_size = st.st_size;
        if (length) *length = file_size;
        
        // 映射文件
        void* ptr = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        
        if (ptr == MAP_FAILED) {
            perror("映射文件失败");
            return nullptr;
        }
        
        return ptr;
    }
    
    // 取消映射
    static bool deallocate(void* ptr, size_t size) {
        return munmap(ptr, size) == 0;
    }
    
    // 设置内存保护
    static bool protect(void* ptr, size_t size, int prot) {
        return mprotect(ptr, size, prot) == 0;
    }
    
    // 同步内存到文件
    static bool sync(void* ptr, size_t size, bool async = false) {
        int flags = async ? MS_ASYNC : MS_SYNC;
        return msync(ptr, size, flags) == 0;
    }
};

void demonstrate_anonymous_mmap() {
    std::cout << "=== 匿名 mmap 演示 ===" << std::endl;
    
    const size_t size = 4096;  // 4KB
    std::cout << "分配 " << size << " 字节匿名内存" << std::endl;
    
    // 分配内存
    void* ptr = MMapAllocator::allocate_anonymous(size);
    if (!ptr) return;
    
    std::cout << "分配地址: " << ptr << std::endl;
    
    // 使用内存
    int* data = static_cast<int*>(ptr);
    for (size_t i = 0; i < size / sizeof(int); ++i) {
        data[i] = i * 2;
    }
    
    std::cout << "前10个元素: ";
    for (int i = 0; i < 10; ++i) {
        std::cout << data[i] << " ";
    }
    std::cout << std::endl;
    
    // 测试内存保护
    std::cout << "\n测试内存保护:" << std::endl;
    
    // 设置为只读
    if (MMapAllocator::protect(ptr, size, PROT_READ)) {
        std::cout << "设为只读成功" << std::endl;
        
        // 尝试写入（会触发段错误）
        // data[0] = 999;  // 这会崩溃
        
        // 改回可写
        MMapAllocator::protect(ptr, size, PROT_READ | PROT_WRITE);
        data[0] = 999;  // 现在可以写入
        std::cout << "修改成功: data[0] = " << data[0] << std::endl;
    }
    
    // 释放内存

    //if (MMapAllocator::deallocate(ptr, size)) {
    //    std::cout << "内存释放成功" << std::endl;
    //}
}

void demonstrate_file_mmap() {
    std::cout << "\n=== 文件 mmap 演示 ===" << std::endl;
    
    // 创建测试文件
    const char* filename = "test_mmap_file.txt";
    const char* content = "这是mmap文件映射测试！\n第二行内容\n第三行内容";
    
    // 写入测试文件
    int fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("创建文件失败");
        return;
    }
    
    write(fd, content, strlen(content));
    close(fd);
    
    // 映射文件到内存
    size_t file_size = 0;
    char* file_data = static_cast<char*>(MMapAllocator::map_file(filename, &file_size));
    
    if (file_data) {
        std::cout << "文件大小: " << file_size << " 字节" << std::endl;
        std::cout << "文件内容:\n" << std::string(file_data, file_size) << std::endl;
        
        // 取消映射
//        MMapAllocator::deallocate(file_data, file_size);
    }
    
    // 删除测试文件
//    unlink(filename);
}

void demonstrate_shared_memory() {
    std::cout << "\n=== 共享内存演示 ===" << std::endl;
    
    const char* shm_name = "/my_shared_memory";
    const size_t shm_size = 4096;
    
    // 创建共享内存
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("创建共享内存失败");
        return;
    }
    
    // 设置共享内存大小
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("设置共享内存大小失败");
        close(shm_fd);
        return;
    }
    
    // 映射共享内存
    void* shm_ptr = mmap(nullptr, shm_size, 
                        PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (shm_ptr == MAP_FAILED) {
        perror("映射共享内存失败");
        return;
    }
    
    std::cout << "共享内存地址: " << shm_ptr << std::endl;
    
    // 写入数据
    char* data = static_cast<char*>(shm_ptr);
    strcpy(data, "Hello from process!");
    std::cout << "写入共享内存: " << data << std::endl;
    
    // 从另一个"进程"读取（这里模拟）
    char buffer[100];
    strcpy(buffer, data);
    std::cout << "从共享内存读取: " << buffer << std::endl;
    
    // 清理
//    munmap(shm_ptr, shm_size);
//    shm_unlink(shm_name);
    std::cout << "共享内存已清理" << std::endl;
}

int main() {
    demonstrate_anonymous_mmap();
    demonstrate_file_mmap();
    demonstrate_shared_memory();
    return 0;
}
