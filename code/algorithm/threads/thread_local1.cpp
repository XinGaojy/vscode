#if 0

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

#if 1
#include <iostream>
#include <thread>
#include <chrono>

thread_local int tls = 0;

void worker(int id) {
    tls = id;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "thread " << id << " tls=" << tls << '\n';
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();
    return 0;
}



#endif



