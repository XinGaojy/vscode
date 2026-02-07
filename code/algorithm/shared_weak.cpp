//使用weak_ptr解决shared_ptr来解决循环引用问题

#if 0
#include<iostream>
#include<memory>
using namespace std;

#if 0
//存在内存泄露
class B;
class A{
public:
  shared_ptr<B>ptrB;
  A(){cout<<"A_construct"<<endl;}
  ~A(){cout<<"A_distruct"<<endl;}
};

class B{
public:
  shared_ptr<A>ptrA;
  B(){cout<<"B_construct"<<endl;}
  ~B(){cout<<"B_distruct"<<endl;}
};

#endif





//使用weak_ptr来解决内存泄露,可以只是用一个weak_ptr,也可以使用两个weak_ptr;
#if 1
class B;
class A{
public:
  //unique_ptr<B>ptrB;错误-->资源不能共享
  weak_ptr<B>ptrB;
  A(){cout<<"A_construct"<<endl;}
  ~A(){cout<<"A_distruct"<<endl;}
};
class B{
public:
  //weak_ptr<A>ptrA;
  shared_ptr<A>ptrA;
  B(){cout<<"B_construct"<<endl;}
  ~B(){cout<<"B_distruct"<<endl;}
};




#endif





int main(){

  {
      
    //使用make_shared来保证原子性
#if 0


    shared_ptr<A>pA=make_shared<A>();
    shared_ptr<B>pB=make_shared<B>();


#endif

    //使用new的方式,不是原子性
    shared_ptr<A>pA=shared_ptr<A>(new A());
    shared_ptr<B>pB=shared_ptr<B>(new B());
    cout<<pA.use_count()<<endl;
    cout<<pB.use_count()<<endl;

    pA->ptrB=pB;
    pB->ptrA=pA;

    
    cout<<pA.use_count()<<endl;
    cout<<pB.use_count()<<endl; 
  }

  return 0;
}


#endif




//使用enable_shared_from_this而不是使用shared_ptr<class>(this);

#if 0

#include<iostream>
#include<memory>
using  namespace std;
class A: public enable_shared_from_this<A>{
public:
  
  shared_ptr<A> getShared(){
    return shared_from_this();
  }
  
  shared_ptr<A>getShared1(){
    return shared_ptr<A>(this);//会导致多次释放内存块,需要注意
  }
  
  shared_ptr<A>getShared2(const shared_ptr<A>& other){
    return other;
  }

  A(){cout<<"construct"<<endl;}
  ~A(){cout<<"distruct"<<endl;}

};

int main(){
  shared_ptr<A>ptrA=make_shared<A>();
//  shared_ptr<A>ptrA=shared_ptr<A>(new A());
  //shared_ptr<A>ptrA1=ptrA->getShared1();
  shared_ptr<A>ptrA2 = ptrA->getShared2(ptrA);
  return 0;
}

#endif





//线程安全性share_ptr,unique_ptr,weak_ptr;
#include<algorithm>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <queue>
#include <condition_variable>
#include <optional>
#include <unordered_map>
#include <shared_mutex>

// ==================== shared_ptr 线程安全性演示 ====================

void test_shared_ptr_basic_safety() {
    std::cout << "\n=== 1. shared_ptr 基本线程安全性测试 ===\n";
    std::cout << "说明：shared_ptr的引用计数操作是原子的\n";
    
    auto sharedData = std::make_shared<int>(100);
    std::cout << "初始引用计数: " << sharedData.use_count() << "\n";
    
    std::vector<std::thread> threads;
    std::atomic<int> totalCopies{0};
    
    // 创建多个线程同时拷贝shared_ptr
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([sharedData, i, &totalCopies]() {
            std::ostringstream oss;
            oss << "线程" << i << "启动，当前引用计数: " 
                << sharedData.use_count() << "\n";
            std::cout << oss.str();
            
            // 每个线程创建多个临时副本
            for (int j = 0; j < 1000; ++j) {
                auto localCopy = sharedData;  // 引用计数原子增加
                totalCopies++;
                std::this_thread::yield();
            }
            
            // 离开作用域，localCopy销毁，引用计数原子减少
        });
    }
    
    // 主线程也创建一些副本
    for (int i = 0; i < 1000; ++i) {
        auto tempCopy = sharedData;
        totalCopies++;
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "最终引用计数: " << sharedData.use_count() << "\n";
    std::cout << "总共创建了 " << totalCopies << " 个临时副本\n";
    std::cout << "✓ shared_ptr引用计数操作是线程安全的\n";
}

void test_shared_ptr_object_unsafe() {
    std::cout << "\n=== 2. shared_ptr指向的对象访问不是线程安全的 ===\n";
    
    struct Counter {
        int value = 0;
        void increment() { value++; }
    };
    
    auto sharedCounter = std::make_shared<Counter>();
    constexpr int INCREMENT_COUNT = 10000;
    constexpr int THREAD_COUNT = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> completedThreads{0};
    
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([sharedCounter, i, &completedThreads]() {
            for (int j = 0; j < INCREMENT_COUNT; ++j) {
                sharedCounter->value++;  // 非原子操作，有数据竞争！
            }
            completedThreads++;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int expected = THREAD_COUNT * INCREMENT_COUNT;
    std::cout << "期望值: " << expected << "\n";
    std::cout << "实际值: " << sharedCounter->value << "\n";
    std::cout << (sharedCounter->value == expected ? "✓ 正确" : "✗ 数据竞争！") << "\n";
    std::cout << "说明：shared_ptr只保护引用计数，不保护指向的对象\n";
}

void test_shared_ptr_safe_access() {
    std::cout << "\n=== 3. 安全访问shared_ptr指向的对象 ===\n";
    
    class ThreadSafeCounter {
    private:
        std::shared_ptr<int> counter = std::make_shared<int>(0);
        mutable std::mutex mtx;
        
    public:
        void increment() {
            std::lock_guard<std::mutex> lock(mtx);
            (*counter)++;
        }
        
        int get() const {
            std::lock_guard<std::mutex> lock(mtx);
            return *counter;
        }
        
        // 原子地替换整个计数器
        void replace(int newValue) {
            std::lock_guard<std::mutex> lock(mtx);
            counter = std::make_shared<int>(newValue);
        }
        
        // 获取shared_ptr的副本（线程安全）
        std::shared_ptr<int> get_shared() const {
            std::lock_guard<std::mutex> lock(mtx);
            return counter;  // 引用计数原子增加
        }
    };
    
    ThreadSafeCounter safeCounter;
    constexpr int INCREMENT_COUNT = 10000;
    constexpr int THREAD_COUNT = 10;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&safeCounter, i]() {
            for (int j = 0; j < INCREMENT_COUNT; ++j) {
                safeCounter.increment();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int result = safeCounter.get();
    int expected = THREAD_COUNT * INCREMENT_COUNT;
    
    std::cout << "期望值: " << expected << "\n";
    std::cout << "实际值: " << result << "\n";
    std::cout << (result == expected ? "✓ 线程安全" : "✗ 不安全") << "\n";
    std::cout << "说明：使用mutex保护对对象的访问\n";
}

void test_atomic_shared_ptr_exchange() {
    std::cout << "\n=== 4. shared_ptr原子交换 ===\n";
    std::cout << "注意：C++20提供了atomic<shared_ptr>，这里模拟类似功能\n";
    
    class AtomicSharedPtr {
    private:
        std::shared_ptr<std::string> ptr;
        mutable std::mutex mtx;
        
    public:
        AtomicSharedPtr(std::shared_ptr<std::string> p = nullptr) : ptr(p) {}
        
        // 原子加载
        std::shared_ptr<std::string> load() const {
            std::lock_guard<std::mutex> lock(mtx);
            return ptr;
        }
        
        // 原子存储
        void store(std::shared_ptr<std::string> p) {
            std::lock_guard<std::mutex> lock(mtx);
            ptr = std::move(p);
        }
        
        // 原子交换
        std::shared_ptr<std::string> exchange(std::shared_ptr<std::string> p) {
            std::lock_guard<std::mutex> lock(mtx);
            std::shared_ptr<std::string> old = std::move(ptr);
            ptr = std::move(p);
            return old;
        }
        
        // 比较并交换
        bool compare_exchange(std::shared_ptr<std::string>& expected, 
                              std::shared_ptr<std::string> desired) {
            std::lock_guard<std::mutex> lock(mtx);
            
            if (ptr == expected) {
                ptr = std::move(desired);
                return true;
            }
            expected = ptr;
            return false;
        }
    };
    
    AtomicSharedPtr atomicPtr(std::make_shared<std::string>("初始值"));
    
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&atomicPtr, i, &successCount]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 10));
            
            auto current = atomicPtr.load();
            auto desired = std::make_shared<std::string>(
                "线程" + std::to_string(i) + "设置");
            
            if (atomicPtr.compare_exchange(current, desired)) {
                std::cout << "线程" << i << " CAS成功\n";
                successCount++;
            } else {
                std::cout << "线程" << i << " CAS失败\n";
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "最终值: " << *atomicPtr.load() << "\n";
    std::cout << "CAS成功次数: " << successCount << "\n";
    std::cout << "✓ 实现了原子shared_ptr操作\n";
}

// ==================== unique_ptr 线程安全性演示 ====================

void test_unique_ptr_basic_unsafe() {
    std::cout << "\n=== 5. unique_ptr 基本不是线程安全的 ===\n";
    
    std::unique_ptr<int> uniquePtr = std::make_unique<int>(100);
    
    // 尝试在多个线程中移动unique_ptr（这是不安全的！）
    std::atomic<bool> stop{false};
    std::atomic<int> readAttempts{0};
    std::atomic<int> moveAttempts{0};
    
    auto reader = [&uniquePtr, &stop, &readAttempts]() {
        while (!stop.load()) {
            // 错误：多个线程读取同一个unique_ptr
            if (uniquePtr) {
                readAttempts++;
            }
            std::this_thread::yield();
        }
    };
    
    auto mover = [&uniquePtr, &moveAttempts]() {
        // 错误：移动unique_ptr
        auto local = std::move(uniquePtr);
        moveAttempts++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        uniquePtr = std::move(local);
    };
    
    std::thread t1(reader);
    std::thread t2(reader);
    
    // 注意：这里我们注释掉mover的调用，因为它是未定义行为
    // std::thread t3(mover);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    stop = true;
    
    t1.join();
    t2.join();
    // t3.join();
    
    std::cout << "读取尝试次数: " << readAttempts << "\n";
    std::cout << "移动尝试次数: " << moveAttempts << "\n";
    std::cout << "✗ unique_ptr在多线程中直接使用是不安全的\n";
    std::cout << "原因：unique_ptr的移动操作不是原子的\n";
}

void test_unique_ptr_safe_patterns() {
    std::cout << "\n=== 6. unique_ptr 安全使用模式 ===\n";
    
    // 模式1：每个线程独立的unique_ptr
    std::cout << "\n模式1：每个线程有独立的unique_ptr\n";
    {
        auto worker = [](int id) {
            thread_local std::unique_ptr<int> threadData = 
                std::make_unique<int>(id * 1000);
            
            for (int i = 0; i < 3; ++i) {
                (*threadData)++;
                std::cout << "线程" << id << " 数据: " << *threadData << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back(worker, i);
        }
        
        for (auto& t : threads) {
            t.join();
        }
        std::cout << "✓ 每个线程独立unique_ptr是安全的\n";
    }
    
    // 模式2：通过队列传递所有权
    std::cout << "\n模式2：通过队列传递unique_ptr所有权\n";
    {
        class ThreadSafeQueue {
        private:
            std::queue<std::unique_ptr<int>> queue;
            std::mutex mtx;
            std::condition_variable cv;
            
        public:
            void push(std::unique_ptr<int> value) {
                std::lock_guard<std::mutex> lock(mtx);
                queue.push(std::move(value));
                cv.notify_one();
            }
            
            std::unique_ptr<int> pop() {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !queue.empty(); });
                
                auto value = std::move(queue.front());
                queue.pop();
                return value;
            }
            
            bool empty()  {
                std::lock_guard<std::mutex> lock(mtx);
                return queue.empty();
            }
        };
        
        ThreadSafeQueue safeQueue;
        std::atomic<int> processedCount{0};
        
        // 生产者
        auto producer = [&safeQueue]() {
            for (int i = 0; i < 5; ++i) {
                auto data = std::make_unique<int>(i * 100);
                std::cout << "生产数据: " << *data << "\n";
                safeQueue.push(std::move(data));
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        };
        
        // 消费者
        auto consumer = [&safeQueue, &processedCount](int id) {
            while (processedCount < 5) {
                auto data = safeQueue.pop();
                if (data) {
                    std::cout << "消费者" << id << " 处理: " << *data << "\n";
                    processedCount++;
                }
            }
        };
        
        std::thread prod(producer);
        std::thread cons1(consumer, 1);
        std::thread cons2(consumer, 2);
        
        prod.join();
        cons1.join();
        cons2.join();
        
        std::cout << "✓ 通过队列传递所有权是安全的\n";
    }
    
    // 模式3：使用mutex包装unique_ptr
    std::cout << "\n模式3：使用mutex包装unique_ptr\n";
    {
        class ThreadSafeUniquePtr {
        private:
            std::unique_ptr<int> data;
            mutable std::mutex mtx;
            
        public:
            ThreadSafeUniquePtr(std::unique_ptr<int> d = nullptr) 
                : data(std::move(d)) {}
            
            // 获取值（只读）
            std::optional<int> get() const {
                std::lock_guard<std::mutex> lock(mtx);
                if (data) {
                    return *data;
                }
                return std::nullopt;
            }
            
            // 设置值
            void set(int value) {
                std::lock_guard<std::mutex> lock(mtx);
                if (!data) {
                    data = std::make_unique<int>(value);
                } else {
                    *data = value;
                }
            }
            
            // 获取所有权
            std::unique_ptr<int> take() {
                std::lock_guard<std::mutex> lock(mtx);
                return std::move(data);
            }
            
            // 设置所有权
            void give(std::unique_ptr<int> newData) {
                std::lock_guard<std::mutex> lock(mtx);
                data = std::move(newData);
            }
        };
        
        ThreadSafeUniquePtr safePtr(std::make_unique<int>(999));
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&safePtr, i]() {
                for (int j = 0; j < 3; ++j) {
                    auto value = safePtr.get();
                    if (value) {
                        std::cout << "线程" << i << " 读取: " << *value << "\n";
                    }
                    safePtr.set(i * 100 + j);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        std::cout << "✓ 使用mutex包装是安全的\n";
    }
}

// ==================== weak_ptr 线程安全性演示 ====================

void test_weak_ptr_basic_safety() {
    std::cout << "\n=== 7. weak_ptr 基本线程安全性测试 ===\n";
    
    std::shared_ptr<int> shared = std::make_shared<int>(42);
    std::weak_ptr<int> weak = shared;
    
    std::cout << "初始状态:\n";
    std::cout << "  shared_ptr引用计数: " << shared.use_count() << "\n";
    std::cout << "  weak_ptr是否过期: " << (weak.expired() ? "是" : "否") << "\n";
    
    std::vector<std::thread> threads;
    std::atomic<int> lockSuccessCount{0};
    std::atomic<int> expiredCount{0};
    
    // 创建多个线程同时访问weak_ptr
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([weak, i, &lockSuccessCount, &expiredCount]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 20));
            
            // 尝试lock
            if (auto locked = weak.lock()) {
                std::ostringstream oss;
                oss << "线程" << i << " lock成功, 值: " << *locked 
                    << ", 引用计数: " << locked.use_count() << "\n";
                std::cout << oss.str();
                lockSuccessCount++;
            } else {
                std::cout << "线程" << i << " lock失败（对象已销毁）\n";
                expiredCount++;
            }
        });
    }
    
    // 主线程稍后释放shared_ptr
    std::thread releaser([&shared]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        std::cout << "\n主线程释放shared_ptr...\n";
        shared.reset();
        std::cout << "shared_ptr已释放\n";
    });
    
    for (auto& t : threads) {
        t.join();
    }
    releaser.join();
    
    std::cout << "\n最终统计:\n";
    std::cout << "  lock成功次数: " << lockSuccessCount << "\n";
    std::cout << "  lock失败次数: " << expiredCount << "\n";
    std::cout << "  weak_ptr是否过期: " << (weak.expired() ? "是" : "否") << "\n";
    std::cout << "✓ weak_ptr的lock()是线程安全的\n";
}

void test_weak_ptr_toctou_race() {
    std::cout << "\n=== 8. weak_ptr TOCTOU竞态条件 ===\n";
    std::cout << "TOCTOU: Time-Of-Check-Time-Of-Use 检查时与使用时之间的竞态条件\n";
    
    std::shared_ptr<int> shared = std::make_shared<int>(100);
    std::weak_ptr<int> weak = shared;
    
    std::atomic<bool> stop{false};
    std::atomic<int> toctouErrors{0};
    std::atomic<int> correctUses{0};
    
    // 错误的用法：先检查后使用
    auto wrong_usage = [&weak, &toctouErrors, &stop]() {
        while (!stop.load()) {
            // 错误：检查和使用不是原子的
            if (!weak.expired()) {  // 检查
                // 这里其他线程可能已经释放了shared_ptr
                auto locked = weak.lock();  // 使用
                if (locked) {
                    // 安全的操作
                } else {
                    toctouErrors++;
                }
            }
            std::this_thread::yield();
        }
    };
    
    // 正确的用法：直接使用lock
    auto correct_usage = [&weak, &correctUses, &stop]() {
        while (!stop.load()) {
            // 正确：单次原子操作
            if (auto locked = weak.lock()) {
                // 安全的操作
                correctUses++;
            }
            std::this_thread::yield();
        }
    };
    
    // 释放者线程
    std::thread releaser([&shared, &stop]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        shared.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        stop = true;
    });
    
    // 测试错误的用法
    std::cout << "测试错误用法（先检查后使用）...\n";
    std::thread wrongThread(wrong_usage);
    
    // 测试正确的用法
    std::cout << "测试正确用法（直接使用lock）...\n";
    std::thread correctThread(correct_usage);
    
    wrongThread.join();
    correctThread.join();
    releaser.join();
    
    std::cout << "\n统计结果:\n";
    std::cout << "  TOCTOU错误次数: " << toctouErrors << "\n";
    std::cout << "  正确使用次数: " << correctUses << "\n";
    std::cout << "✗ 错误用法可能导致悬垂指针访问\n";
    std::cout << "✓ 正确用法是安全的\n";
}

void test_weak_ptr_cache_example() {
    std::cout << "\n=== 9. weak_ptr 在缓存中的实际应用 ===\n";
    
    class ThreadSafeCache {
    private:
        struct CacheEntry {
            std::weak_ptr<std::string> data;
            std::chrono::steady_clock::time_point timestamp;
            
            bool is_expired(std::chrono::seconds ttl) const {
                auto now = std::chrono::steady_clock::now();
                return (now - timestamp) > ttl;
            }
        };
        
        std::unordered_map<std::string, CacheEntry> cache;
        mutable std::shared_mutex cache_mutex;
        std::chrono::seconds default_ttl{5};  // 5秒TTL
        
    public:
        // 获取缓存
        std::shared_ptr<std::string> get(const std::string& key) {
            // 尝试获取读锁
            {
                std::shared_lock<std::shared_mutex> lock(cache_mutex);
                auto it = cache.find(key);
                if (it != cache.end() && !it->second.is_expired(default_ttl)) {
                    if (auto data = it->second.data.lock()) {
                        std::cout << "缓存命中: " << key << "\n";
                        return data;  // 返回强引用
                    }
                }
            }
            
            // 缓存未命中，需要创建
            std::unique_lock<std::shared_mutex> lock(cache_mutex);
            
            // 双重检查（避免竞态条件）
            auto it = cache.find(key);
            if (it != cache.end()) {
                if (!it->second.is_expired(default_ttl)) {
                    if (auto data = it->second.data.lock()) {
                        return data;
                    }
                }
                // 清理过期的weak_ptr
                cache.erase(it);
            }
            
            // 创建新数据
            auto new_data = std::make_shared<std::string>("数据: " + key);
            cache[key] = {new_data, std::chrono::steady_clock::now()};
            
            std::cout << "创建缓存: " << key << "\n";
            return new_data;
        }
        
        // 清理过期缓存
        void cleanup() {
            std::unique_lock<std::shared_mutex> lock(cache_mutex);
            auto now = std::chrono::steady_clock::now();
            
            for (auto it = cache.begin(); it != cache.end();) {
                if (it->second.data.expired() || 
                    (now - it->second.timestamp) > default_ttl) {
                    std::cout << "清理过期缓存: " << it->first << "\n";
                    it = cache.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        // 获取缓存统计
        void print_stats() const {
            std::shared_lock<std::shared_mutex> lock(cache_mutex);
            int total = 0;
            int valid = 0;
            int expired = 0;
            
            auto now = std::chrono::steady_clock::now();
            for (const auto& entry : cache) {
                total++;
                if (entry.second.data.expired()) {
                    expired++;
                } else if (entry.second.is_expired(default_ttl)) {
                    expired++;
                } else {
                    valid++;
                }
            }
            
            std::cout << "缓存统计: 总计=" << total 
                     << ", 有效=" << valid 
                     << ", 过期=" << expired << "\n";
        }
    };
    
    ThreadSafeCache cache;
    std::vector<std::thread> threads;
    
    // 创建多个线程并发访问缓存
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, i]() {
            for (int j = 0; j < 3; ++j) {
                std::string key = "key" + std::to_string(j);
                auto data = cache.get(key);
                
                std::ostringstream oss;
                oss << "线程" << i << " 获取 " << key 
                    << " -> " << (data ? *data : "null") << "\n";
                std::cout << oss.str();
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }
    
    // 清理线程
    std::thread cleaner([&cache]() {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            cache.cleanup();
            cache.print_stats();
        }
    });
    
    for (auto& t : threads) {
        t.join();
    }
    cleaner.join();
    
    std::cout << "\n最终缓存状态:\n";
    cache.print_stats();
    cache.cleanup();
    cache.print_stats();
    
    std::cout << "✓ weak_ptr非常适合实现自动清理的缓存\n";
}

// ==================== 综合示例 ====================

void test_comprehensive_example() {
    std::cout << "\n=== 10. 综合示例：智能指针在生产环境的应用 ===\n";
    
    class ConnectionPool {
    private:
        struct Connection {
            int id;
            bool in_use = false;
            std::chrono::steady_clock::time_point last_used;
            
            Connection(int conn_id) : id(conn_id) {
                last_used = std::chrono::steady_clock::now();
                std::cout << "  创建连接 " << id << "\n";
            }
            
            ~Connection() {
                std::cout << "  销毁连接 " << id << "\n";
            }
            
            void query(const std::string& sql) {
                std::cout << "  连接" << id << " 执行: " << sql << "\n";
                last_used = std::chrono::steady_clock::now();
            }
        };
        
        std::vector<std::shared_ptr<Connection>> pool;
        mutable std::mutex pool_mutex;
        std::condition_variable pool_cv;
        int next_id = 1;
        const int max_pool_size = 5;
        
        // 自定义删除器，将连接返回到池中
        struct ConnectionDeleter {
            ConnectionPool* pool;
            
            void operator()(Connection* conn) {
                if (pool && conn) {
                    pool->return_connection(std::unique_ptr<Connection>(conn));
                } else {
                    delete conn;
                }
            }
        };
        
    public:
        ConnectionPool() {
            std::cout << "初始化连接池，最大大小: " << max_pool_size << "\n";
        }
        
        ~ConnectionPool() {
            std::cout << "销毁连接池，剩余连接: " << pool.size() << "\n";
        }
        
        // 获取连接
        std::shared_ptr<Connection> acquire_connection() {
            std::unique_lock<std::mutex> lock(pool_mutex);
            
            // 尝试从池中获取可用连接
            for (auto& conn : pool) {
                if (!conn->in_use) {
                    conn->in_use = true;
                    std::cout << "从池中获取连接 " << conn->id << "\n";
                    return {conn.get(), ConnectionDeleter{this}};
                }
            }
            
            // 池中没有可用连接，检查是否可创建新连接
            if (pool.size() < max_pool_size) {
                auto new_conn = std::make_shared<Connection>(next_id++);
                new_conn->in_use = true;
                pool.push_back(new_conn);
                std::cout << "创建新连接 " << new_conn->id << "\n";
                return {new_conn.get(), ConnectionDeleter{this}};
            }
            
            // 等待连接释放
            std::cout << "连接池已满，等待...\n";
            pool_cv.wait(lock, [this] {
                for (const auto& conn : pool) {
                    if (!conn->in_use) return true;
                }
                return false;
            });
            
            // 获取释放的连接
            for (auto& conn : pool) {
                if (!conn->in_use) {
                    conn->in_use = true;
                    std::cout << "获取释放的连接 " << conn->id << "\n";
                    return {conn.get(), ConnectionDeleter{this}};
                }
            }
            
            return nullptr;
        }
        
    private:
        // 返回连接到池中
        void return_connection(std::unique_ptr<Connection> conn) {
            if (!conn) return;
            
            std::unique_lock<std::mutex> lock(pool_mutex);
            conn->in_use = false;
            conn->last_used = std::chrono::steady_clock::now();
            std::cout << "连接 " << conn->id << " 返回池中\n";
            
            // 放弃unique_ptr的所有权
            conn.release();
            
            // 通知等待的线程
            pool_cv.notify_one();
        }
        
    public:
        // 清理空闲连接
        void cleanup_idle_connections(std::chrono::seconds idle_timeout) {
            std::unique_lock<std::mutex> lock(pool_mutex);
            auto now = std::chrono::steady_clock::now();
            
            pool.erase(
                std::remove_if(pool.begin(), pool.end(),
                    [&](const std::shared_ptr<Connection>& conn) {
                        if (!conn->in_use && 
                            (now - conn->last_used) > idle_timeout) {
                            std::cout << "清理空闲连接 " << conn->id << "\n";
                            return true;
                        }
                        return false;
                    }),
                pool.end()
            );
        }
    };
    
    ConnectionPool pool;
    std::vector<std::thread> workers;
    std::atomic<int> query_count{0};
    
    // 创建多个工作线程
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([&pool, i, &query_count]() {
            for (int j = 0; j < 2; ++j) {
                auto conn = pool.acquire_connection();
                if (conn) {
                    std::ostringstream sql;
                    sql << "SELECT * FROM table WHERE id = " << (i * 10 + j);
                    conn->query(sql.str());
                    query_count++;
                    
                    // 模拟查询时间
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    
                    // conn离开作用域，自动返回池中
                }
            }
        });
    }
    
    // 清理线程
    std::thread cleaner([&pool]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "\n清理空闲连接...\n";
        pool.cleanup_idle_connections(std::chrono::seconds(1));
    });
    
    for (auto& t : workers) {
        t.join();
    }
    cleaner.join();
    
    std::cout << "\n执行查询总数: " << query_count << "\n";
    std::cout << "✓ 综合使用了shared_ptr、weak_ptr和unique_ptr的特性\n";
}

// ==================== 主测试函数 ====================

int main() {
    std::cout << "========== C++智能指针线程安全性演示 ==========\n";
    
    // shared_ptr 测试
    test_shared_ptr_basic_safety();
    test_shared_ptr_object_unsafe();
    test_shared_ptr_safe_access();
    test_atomic_shared_ptr_exchange();
    
    // unique_ptr 测试
    test_unique_ptr_basic_unsafe();
//    test_unique_ptr_safe_patterns();
    
    // weak_ptr 测试
    test_weak_ptr_basic_safety();
    test_weak_ptr_toctou_race();
    test_weak_ptr_cache_example();
    
    // 综合示例
    test_comprehensive_example();
    
    std::cout << "\n========== 测试完成 ==========\n";
    
    return 0;
}








