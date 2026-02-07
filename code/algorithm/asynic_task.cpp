//实现一个简单的异步任务并获取计算结果


#include <iostream>
#include <future>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include<chrono>
#include<thread>
// 简单的异步任务管理器
class AsyncTaskManager {
private:
    std::vector<std::future<void>> tasks;
    
public:
    // 添加异步任务
    template<typename Func, typename... Args>
    auto addTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
        // 获取函数返回类型
        using ReturnType = decltype(func(args...));
        
        // 创建 packaged_task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );
        
        // 获取 future
        std::future<ReturnType> result = task->get_future();
        
        // 在新线程中执行
        std::thread([task]() {
            (*task)();
        }).detach();
        
        return result;
    }
    
    // 等待所有任务完成
    void waitAll() {
        // 对于这个简单实现，我们假设任务都会在合适时机完成
        // 实际应用中需要更复杂的同步机制
    }
};

// 测试函数
std::string fetchData(const std::string& url) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return "Data from " + url;
}

int calculateSum(const std::vector<int>& nums) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    int sum = 0;
    for (int num : nums) {
        sum += num;
    }
    return sum;
}

int main() {
    AsyncTaskManager taskManager;
    
    // 并行执行多个任务
    auto dataFuture = taskManager.addTask(fetchData, "https://example.com/api");
    auto sumFuture = taskManager.addTask(calculateSum, std::vector<int>{1, 2, 3, 4, 5});
    
    // 主线程继续执行
    std::cout << "主线程继续执行其他操作..." << std::endl;
    
    // 获取结果
    std::string data = dataFuture.get();
    int sum = sumFuture.get();
    
    std::cout << "获取的数据: " << data << std::endl;
    std::cout << "计算的和: " << sum << std::endl;
    
    return 0;
}
