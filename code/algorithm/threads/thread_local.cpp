#if 0

#include <iostream>
#include <thread>
#include <vector>

// 线程局部变量
thread_local int thread_specific_var = 0;

void thread_function(int id) {
    thread_specific_var = id * 100;  // 每个线程有自己的副本
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "线程 " << id << ": thread_specific_var = " 
              << thread_specific_var << std::endl;
}

void thread_local_storage() {
    std::cout << "\n=== 线程局部存储 ===" << std::endl;
    
    // 在主线程中设置
    thread_specific_var = 999;
    std::cout << "主线程: thread_specific_var = " 
              << thread_specific_var << std::endl;
    
    // 创建多个线程
    std::vector<std::thread> threads;
    for (int i = 1; i <= 5; ++i) {
        threads.emplace_back(thread_function, i);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 主线程的变量不受影响
    std::cout << "主线程: thread_specific_var = " 
              << thread_specific_var << std::endl;
    
    // thread_local 特点：
    // 1. 每个线程有自己的副本
    // 2. 线程创建时初始化，线程结束时销毁
    // 3. 可以用于避免竞争条件
    // 4. 性能比普通静态变量差
}

int main(){
  thread_local_storage();

  return 0;
}


#endif


#if 1
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

thread_local int tls_counter = 42;          // 位于 TLS 段

void show(const char* prefix) {
    // 取 TLS 变量地址 & 线程 ID
    printf("%s tid=%ld  &tls_counter=%p  val=%d\n",
           prefix, syscall(SYS_gettid), &tls_counter, tls_counter);
    ++tls_counter;                          // 只影响本线程副本
}

int main() {
    show("main");
    std::thread t1([]{ show("thr1"); });
    std::thread t2([]{ show("thr2"); });
    t1.join(); t2.join();
    show("main again");                     // 验证主线程副本未变
}


#endif


