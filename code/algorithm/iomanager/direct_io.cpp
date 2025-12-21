#define _GNU_SOURCE
#include <cstdlib>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <memory>
#include <time.h>
#include <sys/time.h>

struct direct_io_ctx {
    int fd;
    size_t block_size;
    size_t alignment;
    void* aligned_buffer;
    size_t buffer_size;
};

// 创建Direct I/O上下文
struct direct_io_ctx* direct_io_open(const char* filename, size_t buffer_size) {
    struct direct_io_ctx* ctx = (struct direct_io_ctx*)malloc(sizeof(*ctx));
    if (!ctx) return NULL;
    
    // 打开文件（Direct I/O模式）
    ctx->fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (ctx->fd == -1) {
        perror("无法以Direct I/O模式打开文件");
        free(ctx);
        return NULL;
    }
    
    // 获取块大小
    struct stat st;
    fstat(ctx->fd, &st);
    ctx->block_size = st.st_blksize;
    ctx->alignment = ctx->block_size;
    
    // 分配对齐的缓冲区
    if (posix_memalign(&ctx->aligned_buffer, ctx->alignment, buffer_size) != 0) {
        perror("内存对齐分配失败");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }
    
    ctx->buffer_size = buffer_size;
    
    printf("Direct I/O上下文创建成功：\n");
    printf("  文件描述符: %d\n", ctx->fd);
    printf("  块大小: %zu 字节\n", ctx->block_size);
    printf("  缓冲区: %p (对齐到 %zu 字节)\n", ctx->aligned_buffer, ctx->alignment);
    printf("  缓冲区大小: %zu 字节\n", ctx->buffer_size);
    
    return ctx;
}

// Direct I/O读取
ssize_t direct_io_read(struct direct_io_ctx* ctx, off_t offset, size_t size) {
    // 检查对齐和大小限制
    if ((uintptr_t)ctx->aligned_buffer % ctx->alignment != 0) {
        errno = EINVAL;
        return -1;
    }
    
    if (size % ctx->block_size != 0) {
        printf("警告: 读取大小 %zu 不是块大小 %zu 的倍数\n", size, ctx->block_size);
        // 仍然尝试，但可能在某些系统上失败
    }
    
    if (offset % ctx->block_size != 0) {
        printf("警告: 偏移量 %ld 不是块对齐的\n", offset);
    }
    
    // 执行Direct I/O读取
    return pread(ctx->fd, ctx->aligned_buffer, size, offset);
}

// Direct I/O写入
ssize_t direct_io_write(struct direct_io_ctx* ctx, off_t offset, size_t size) {
    // 类似的检查和验证
    if ((uintptr_t)ctx->aligned_buffer % ctx->alignment != 0) {
        errno = EINVAL;
        return -1;
    }
    
    return pwrite(ctx->fd, ctx->aligned_buffer, size, offset);
}

// 清理资源
void direct_io_close(struct direct_io_ctx* ctx) {
    if (ctx) {
        if (ctx->fd != -1) {
            close(ctx->fd);
        }
        if (ctx->aligned_buffer) {
            free(ctx->aligned_buffer);
        }
        free(ctx);
    }
}

// 获取高精度时间（微秒）
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

// 同步kernel buffer的上下文
struct buffered_io_ctx {
    int fd;
    char* buffer;
    size_t buffer_size;
};

// 创建缓冲I/O上下文
struct buffered_io_ctx* buffered_io_open(const char* filename, size_t buffer_size) {
    struct buffered_io_ctx* ctx = (struct buffered_io_ctx*)malloc(sizeof(*ctx));
    if (!ctx) return NULL;
    
    // 打开文件（标准缓冲模式）
    ctx->fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (ctx->fd == -1) {
        perror("无法以缓冲模式打开文件");
        free(ctx);
        return NULL;
    }
    
    // 分配普通缓冲区（不需要对齐）
    ctx->buffer = (char*)malloc(buffer_size);
    if (!ctx->buffer) {
        perror("内存分配失败");
        close(ctx->fd);
        free(ctx);
        return NULL;
    }
    
    ctx->buffer_size = buffer_size;
    
    printf("缓冲I/O上下文创建成功：\n");
    printf("  文件描述符: %d\n", ctx->fd);
    printf("  缓冲区大小: %zu 字节\n", ctx->buffer_size);
    
    return ctx;
}

// 缓冲I/O读取
ssize_t buffered_io_read(struct buffered_io_ctx* ctx, off_t offset, size_t size) {
    return pread(ctx->fd, ctx->buffer, size, offset);
}

// 缓冲I/O写入
ssize_t buffered_io_write(struct buffered_io_ctx* ctx, off_t offset, size_t size) {
    return pwrite(ctx->fd, ctx->buffer, size, offset);
}

// 同步kernel buffer到磁盘
int sync_buffered_io(struct buffered_io_ctx* ctx) {
    // 使用fsync将内核缓冲区数据刷到磁盘
    return fsync(ctx->fd);
}

// 清理缓冲I/O资源
void buffered_io_close(struct buffered_io_ctx* ctx) {
    if (ctx) {
        if (ctx->fd != -1) {
            close(ctx->fd);
        }
        if (ctx->buffer) {
            free(ctx->buffer);
        }
        free(ctx);
    }
}

// Direct I/O性能测试
void direct_io_performance_test() {
    const char* filename = "test_direct_io.bin";
    const size_t buffer_size = 64 * 1024; // 64KB
    const int iterations = 100;
    
    printf("\n=== Direct I/O 性能测试 ===\n");
    
    // 创建Direct I/O上下文
    struct direct_io_ctx* ctx = direct_io_open(filename, buffer_size);
    if (!ctx) {
        return;
    }
    
    // 准备测试数据
    memset(ctx->aligned_buffer, 0xAB, buffer_size);
    
    // 写入性能测试
    long long write_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t written = direct_io_write(ctx, i * buffer_size, buffer_size);
        if (written == -1) {
            perror("Direct I/O写入失败");
            break;
        }
    }
    long long write_end = get_time_us();
    
    // 读取性能测试
    long long read_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t read_bytes = direct_io_read(ctx, i * buffer_size, buffer_size);
        if (read_bytes == -1) {
            perror("Direct I/O读取失败");
            break;
        }
    }
    long long read_end = get_time_us();
    
    // 计算吞吐量
    double write_time = (write_end - write_start) / 1000000.0;
    double read_time = (read_end - read_start) / 1000000.0;
    double total_data = (iterations * buffer_size) / (1024.0 * 1024); // MB
    
    double write_throughput = total_data / write_time;
    double read_throughput = total_data / read_time;
    
    printf("Direct I/O 结果：\n");
    printf("  写入: %.2f MB/s (%.3f 秒)\n", write_throughput, write_time);
    printf("  读取: %.2f MB/s (%.3f 秒)\n", read_throughput, read_time);
    printf("  总数据量: %.2f MB\n", total_data);
    
    // 清理
    direct_io_close(ctx);
    unlink(filename);
}

// 缓冲I/O性能测试（无同步）
void buffered_io_performance_test_no_sync() {
    const char* filename = "test_buffered_io.bin";
    const size_t buffer_size = 64 * 1024; // 64KB
    const int iterations = 100;
    
    printf("\n=== 缓冲I/O性能测试（无同步）=== \n");
    printf("注意：数据可能在Page Cache中，未立即写入磁盘\n");
    
    // 创建缓冲I/O上下文
    struct buffered_io_ctx* ctx = buffered_io_open(filename, buffer_size);
    if (!ctx) {
        return;
    }
    
    // 准备测试数据
    memset(ctx->buffer, 0xAB, buffer_size);
    
    // 写入性能测试（不调用fsync）
    long long write_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t written = buffered_io_write(ctx, i * buffer_size, buffer_size);
        if (written == -1) {
            perror("缓冲I/O写入失败");
            break;
        }
    }
    long long write_end = get_time_us();
    
    // 读取性能测试（可能从缓存读取）
    long long read_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t read_bytes = buffered_io_read(ctx, i * buffer_size, buffer_size);
        if (read_bytes == -1) {
            perror("缓冲I/O读取失败");
            break;
        }
    }
    long long read_end = get_time_us();
    
    // 计算吞吐量
    double write_time = (write_end - write_start) / 1000000.0;
    double read_time = (read_end - read_start) / 1000000.0;
    double total_data = (iterations * buffer_size) / (1024.0 * 1024); // MB
    
    double write_throughput = total_data / write_time;
    double read_throughput = total_data / read_time;
    
    printf("缓冲I/O（无同步）结果：\n");
    printf("  写入: %.2f MB/s (%.3f 秒)\n", write_throughput, write_time);
    printf("  读取: %.2f MB/s (%.3f 秒)\n", read_throughput, read_time);
    printf("  总数据量: %.2f MB\n", total_data);
    
    // 清理
    buffered_io_close(ctx);
    unlink(filename);
}

// 缓冲I/O性能测试（带同步）
void buffered_io_performance_test_with_sync() {
    const char* filename = "test_buffered_io_sync.bin";
    const size_t buffer_size = 64 * 1024; // 64KB
    const int iterations = 100;
    
    printf("\n=== 缓冲I/O性能测试（带同步）=== \n");
    printf("每次写入后调用fsync()，确保数据落盘\n");
    
    // 创建缓冲I/O上下文
    struct buffered_io_ctx* ctx = buffered_io_open(filename, buffer_size);
    if (!ctx) {
        return;
    }
    
    // 准备测试数据
    memset(ctx->buffer, 0xAB, buffer_size);
    
    // 写入性能测试（每次写入后同步）
    long long write_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t written = buffered_io_write(ctx, i * buffer_size, buffer_size);
        if (written == -1) {
            perror("缓冲I/O写入失败");
            break;
        }
        
        // 关键区别：每次写入后同步到磁盘
        if (sync_buffered_io(ctx) == -1) {
            perror("fsync失败");
            break;
        }
    }
    long long write_end = get_time_us();
    
    // 清空缓存，确保从磁盘读取
    system("sync; echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
    
    // 读取性能测试（确保从磁盘读取）
    long long read_start = get_time_us();
    for (int i = 0; i < iterations; i++) {
        ssize_t read_bytes = buffered_io_read(ctx, i * buffer_size, buffer_size);
        if (read_bytes == -1) {
            perror("缓冲I/O读取失败");
            break;
        }
    }
    long long read_end = get_time_us();
    
    // 计算吞吐量
    double write_time = (write_end - write_start) / 1000000.0;
    double read_time = (read_end - read_start) / 1000000.0;
    double total_data = (iterations * buffer_size) / (1024.0 * 1024); // MB
    
    double write_throughput = total_data / write_time;
    double read_throughput = total_data / read_time;
    
    printf("缓冲I/O（带同步）结果：\n");
    printf("  写入: %.2f MB/s (%.3f 秒)\n", write_throughput, write_time);
    printf("  读取: %.2f MB/s (%.3f 秒)\n", read_throughput, read_time);
    printf("  总数据量: %.2f MB\n", total_data);
    
    // 清理
    buffered_io_close(ctx);
    unlink(filename);
}

// 综合性能对比测试
void comprehensive_performance_comparison() {
    const char* direct_filename = "test_direct.bin";
    const char* buffered_filename = "test_buffered.bin";
    const char* buffered_sync_filename = "test_buffered_sync.bin";
    
    const size_t buffer_size = 64 * 1024; // 64KB
    const int iterations = 100;
    const int warmup_runs = 3; // 预热运行次数
    
    printf("\n=== 综合性能对比测试 ===\n");
    printf("测试配置：%d次操作，每次%dKB，总数据量：%.2fMB\n\n", 
           iterations, buffer_size/1024, (iterations * buffer_size) / (1024.0 * 1024));
    
    // 预热运行（避免冷启动影响）
    printf("预热运行...\n");
    for (int i = 0; i < warmup_runs; i++) {
        direct_io_performance_test();
        buffered_io_performance_test_no_sync();
        buffered_io_performance_test_with_sync();
    }
    
    // 正式测试
    printf("\n正式性能测试：\n");
    printf("========================================\n");
    
    // 测试1: Direct I/O
    printf("1. Direct I/O测试：\n");
    struct direct_io_ctx* direct_ctx = direct_io_open(direct_filename, buffer_size);
    if (direct_ctx) {
        memset(direct_ctx->aligned_buffer, 0xAB, buffer_size);
        
        long long start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            direct_io_write(direct_ctx, i * buffer_size, buffer_size);
        }
        long long direct_write_time = get_time_us() - start;
        
        start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            direct_io_read(direct_ctx, i * buffer_size, buffer_size);
        }
        long long direct_read_time = get_time_us() - start;
        
        double direct_data_mb = (iterations * buffer_size) / (1024.0 * 1024);
        double direct_write_throughput = direct_data_mb / (direct_write_time / 1000000.0);
        double direct_read_throughput = direct_data_mb / (direct_read_time / 1000000.0);
        
        printf("   写入: %.2f MB/s\n", direct_write_throughput);
        printf("   读取: %.2f MB/s\n", direct_read_throughput);
        
        direct_io_close(direct_ctx);
        unlink(direct_filename);
    }
    
    // 测试2: 缓冲I/O（无同步）
    printf("\n2. 缓冲I/O测试（无同步）：\n");
    struct buffered_io_ctx* buffered_ctx = buffered_io_open(buffered_filename, buffer_size);
    if (buffered_ctx) {
        memset(buffered_ctx->buffer, 0xAB, buffer_size);
        
        long long start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            buffered_io_write(buffered_ctx, i * buffer_size, buffer_size);
        }
        long long buffered_write_time = get_time_us() - start;
        
        start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            buffered_io_read(buffered_ctx, i * buffer_size, buffer_size);
        }
        long long buffered_read_time = get_time_us() - start;
        
        double buffered_data_mb = (iterations * buffer_size) / (1024.0 * 1024);
        double buffered_write_throughput = buffered_data_mb / (buffered_write_time / 1000000.0);
        double buffered_read_throughput = buffered_data_mb / (buffered_read_time / 1000000.0);
        
        printf("   写入: %.2f MB/s (数据在Page Cache中)\n", buffered_write_throughput);
        printf("   读取: %.2f MB/s (从Page Cache读取)\n", buffered_read_throughput);
        
        buffered_io_close(buffered_ctx);
        unlink(buffered_filename);
    }
    
    // 测试3: 缓冲I/O（带同步）
    printf("\n3. 缓冲I/O测试（带同步）：\n");
    struct buffered_io_ctx* buffered_sync_ctx = buffered_io_open(buffered_sync_filename, buffer_size);
    if (buffered_sync_ctx) {
        memset(buffered_sync_ctx->buffer, 0xAB, buffer_size);
        
        long long start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            buffered_io_write(buffered_sync_ctx, i * buffer_size, buffer_size);
            sync_buffered_io(buffered_sync_ctx); // 每次写入后同步
        }
        long long buffered_sync_write_time = get_time_us() - start;
        
        // 清空缓存确保从磁盘读取
        system("sync; echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
        
        start = get_time_us();
        for (int i = 0; i < iterations; i++) {
            buffered_io_read(buffered_sync_ctx, i * buffer_size, buffer_size);
        }
        long long buffered_sync_read_time = get_time_us() - start;
        
        double buffered_sync_data_mb = (iterations * buffer_size) / (1024.0 * 1024);
        double buffered_sync_write_throughput = buffered_sync_data_mb / (buffered_sync_write_time / 1000000.0);
        double buffered_sync_read_throughput = buffered_sync_data_mb / (buffered_sync_read_time / 1000000.0);
        
        printf("   写入: %.2f MB/s (立即落盘)\n", buffered_sync_write_throughput);
        printf("   读取: %.2f MB/s (从磁盘读取)\n", buffered_sync_read_throughput);
        
        buffered_io_close(buffered_sync_ctx);
        unlink(buffered_sync_filename);
    }
    
    printf("========================================\n");
}

// 内核缓冲区同步演示
void kernel_buffer_sync_demo() {
    const char* filename = "sync_demo.bin";
    const size_t data_size = 4096;
    
    printf("\n=== 内核缓冲区同步演示 ===\n");
    
    // 创建测试文件
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("无法创建测试文件");
        return;
    }
    
    char* data = (char*)malloc(data_size);
    memset(data, 0xAA, data_size);
    
    // 演示1: 普通写入（数据在kernel buffer）
    printf("1. 普通写入（数据在kernel buffer）：\n");
    write(fd, data, data_size);
    printf("   数据已写入，但在kernel buffer中，尚未落盘\n");
    
    // 演示2: 使用fsync同步
    printf("2. 调用fsync()同步到磁盘：\n");
    long long start = get_time_us();
    if (fsync(fd) == 0) {
        long long sync_time = get_time_us() - start;
        printf("   同步完成，耗时: %lld 微秒\n", sync_time);
        printf("   数据已保证写入磁盘\n");
    } else {
        perror("fsync失败");
    }
    
    // 演示3: 使用fdatasync（只同步数据，不同步元数据）
    printf("3. 调用fdatasync()同步数据：\n");
    write(fd, data, data_size); // 再次写入
    start = get_time_us();
    if (fdatasync(fd) == 0) {
        long long datasync_time = get_time_us() - start;
        printf("   数据同步完成，耗时: %lld 微秒\n", datasync_time);
        printf("   （元数据可能未同步）\n");
    } else {
        perror("fdatasync失败");
    }
    
    // 演示4: 使用O_SYNC标志（每次写入都同步）
    printf("4. 使用O_SYNC标志（每次写入同步）：\n");
    close(fd);
    fd = open(filename, O_RDWR | O_SYNC, 0644);
    
    start = get_time_us();
    write(fd, data, data_size); // 这次写入会立即同步到磁盘
    long long sync_write_time = get_time_us() - start;
    printf("   O_SYNC写入耗时: %lld 微秒\n", sync_write_time);
    
    // 清理
    free(data);
    close(fd);
    unlink(filename);
    
    printf("\n同步操作对比：\n");
    printf("  - 普通写入 + fsync: 批量同步，适合多次写操作后一次性同步\n");
    printf("  - O_SYNC: 每次写入都同步，保证持久性但性能低\n");
    printf("  - fdatasync: 只同步数据，比fsync稍快\n");
}

int main() {
    printf("磁盘I/O性能对比测试程序\n");
    printf("========================\n");
    
    // 演示内核缓冲区同步机制
    kernel_buffer_sync_demo();
    
    // 综合性能对比测试
    comprehensive_performance_comparison();
    
    printf("\n测试完成！\n");
    return 0;
}
