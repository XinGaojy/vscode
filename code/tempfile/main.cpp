#include "fiber.h"
#include "scheduler.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
using namespace flare;
using namespace std::chrono_literals;

void fiber_task(int id) {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Fiber " << id << " step " << i << std::endl;
        Fiber::yield();  // 主动让出执行权
        std::this_thread::sleep_for(100ms); // 模拟耗时操作
    }
}

int main() {
    // 启动10个纤程
    for (int i = 0; i < 1; ++i) {
        Scheduler::instance().spawn([i] { fiber_task(i); });
    }
    
    // 运行调度器（阻塞直到所有纤程完成）
    Scheduler::instance().run();
    
    std::cout << "All fibers completed" << std::endl;
    return 0;
}
