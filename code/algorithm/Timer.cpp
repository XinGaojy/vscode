



#if 0

#include <map>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <cassert>
#include<vector>
using namespace std;
class TimerManager {
private:
    // 定时器结构体
    struct Timer {
        int id;  // 定时器唯一标识
        std::chrono::steady_clock::time_point expire; // 到期时间点
        std::function<void()> callback;  // 回调函数
        
        // 重载<运算符用于红黑树排序
        bool operator<(const Timer& other) const {
            return expire < other.expire;
        }
    };

    // 数据结构说明：
    // 1. timers_：红黑树（multimap），按到期时间排序，保证快速获取最近到期的定时器
    //    - key: 到期时间（time_point）
    //    - value: Timer对象
    // 2. active_timers_：哈希表，存储定时器ID到红黑树迭代器的映射，实现O(1)查找
    std::multimap<std::chrono::steady_clock::time_point, Timer> timers_;
    std::unordered_map<int, decltype(timers_)::iterator> active_timers_;
    
    // 线程安全控制
    std::mutex mtx_;  // 互斥锁保护数据结构
    std::atomic<int> next_id_{0};  // 原子计数器生成唯一ID

public:
    // 添加定时器
    int addTimer(int delay_ms, std::function<void()> cb) {
        std::lock_guard<std::mutex> lock(mtx_);
        
        // 生成唯一ID（原子操作保证线程安全）
        int id = next_id_++;
        
        // 计算到期时间（当前时间 + 延迟）
        auto expire = std::chrono::steady_clock::now() + 
                     std::chrono::milliseconds(delay_ms);
        
        // 插入红黑树并保存迭代器
        auto iter = timers_.emplace(expire, Timer{id, expire, std::move(cb)});
        active_timers_[id] = iter;
        
        return id;
    }

    // 移除定时器
    void remove(int id) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = active_timers_.find(id);
        if (it != active_timers_.end()) {
            timers_.erase(it->second);  // 从红黑树删除
            active_timers_.erase(it);  // 从哈希表删除
        }
    }

    // 检查并触发到期定时器
    void update() {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mtx_);
        
        while (!timers_.empty()) {
            auto iter = timers_.begin();  // 获取最早到期的定时器
            
            // 如果最早定时器还未到期，结束检查
            if (iter->first > now) break;
            
            // 执行回调（如果存在）
            if (iter->second.callback) {
                iter->second.callback();
            }
            
            // 从哈希表删除引用
            active_timers_.erase(iter->second.id);
            // 从红黑树删除
            timers_.erase(iter);
        }
    }
    
};
void print(){
    for(int i=0;i<100;i++){
        cout<<"hello world"<<endl;
    }
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
void print1(){
    for(int i=0;i<100;i++){
        cout<<"jiayou"<<endl;
    }
}
int main(){
    TimerManager tm;
    tm.addTimer(6000,print);
    tm.addTimer(3000,print1);
    while(1){
        tm.update();
        //tm.remove(0);
        //tm.remove(1);
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}


#endif










class TimerManager{
private:
    struct timer{
        int id;
        std::chrono::stready_clock::timer_point expire;
        function<void()>func;
        bool operator=(const timer& other) const{
            return expire<other.expire;
        }

    };
    unordered_map<std::chrono::stready_clock::timer_point ,timer>timers;
    multimap<int ,decltype(timers)::iterator>active_timers;
    mutex mtx;
    atomic<int>next_id{0};
    public:
    int addTimer(int delay_ms,function<void()>f){
        lock_guard<mutex>lock(mtx);
        auto now=std::chrono::stready_clock::now();
        auto expire=now+std::chrono::milliseconds(delay_ms);
        int id=next_id++;
        auto iter=timers.emplace(expire,timers{id,expire,f});
        active_timers[id]=iter;
        return id;
    }
    void remove(int id){
        lock_guard<mutex>lock(mtx);
        auto iter=active_timers.find(id);
        if(iter!=active_timers.end()){
            timers.erase(iter->second);
            active_timers.erase(iter);
        }

    }
    void update(){
        auto now=std::chrono::stready_clock::now();
        while(1){
            lock_guard<mutex>lock(mtx);
            auto iter=timers.begin();
            if(now<iter->first){
                break;
            }
            if(iter->second.func){
                iter->second.func();
            }
            active_timers.erase(iter.second.id);
            timers.erase(iter);
        }
        
    }
};
int main(){
    TimerManager tm;
    tm.addTimer(1000, []{
        cout<<"hello"<<>endl;
    });
    while(1){
        update();
    }
    return 0;
}





class TimerManager{
private:
    struct timer{
        int id;
        std::chrono::stready_clock::timer_point expire;
        function<void()>func;
        bool operator<(const timer& other)const {
            return expire<other.expire;
        }

    }
    unordered_map<std::chrono::stready_clock::timer_point,timer>timers;
    multimap<int,decltype(timers)::iterator>active_timers;
    atomic<int>next_id{0};
    mutex mtx;
public:
    int addTimer(int delay_ms,function<void()>f){
        auto now=std::chrono::stready_clock::now();
        lock_guard<mutex>lock(mtx);
        auto expire=now+std::chrono::milliseconds(now);
        int id=next_id++;
        auto iter=timers.explace(expire,timer{id,expire,f});
        active_timers[id]=iter;
    }
    void remove(int id){
        lock_guard<mutex>lock(mt
        auto iter=active_timers.find(it):
        if(iter!=active_timers.end()){
            timers.erase(iter.second);
            active_timers.erase(iter);
        }
    }
    void update(){
        lock_guard<mutexlock(mtx);
        auto now=std::chrono::stready_clock::now();
        while(1){
            auto iter=timers.begin();
            if(now<iters.first()){
                break;
            }
            if(iter->second.func){
                iter->second.func();
            }
            active_timers.erase(iter->second.id);
            timers.erase(iter);
        }
    }
};
int main(){
    TimerManager tm;
    tm.addTimer(1000, [this]{
        cout<<"hello"<<endl;
    })
    while(1){
        tm.update();
    }
    return 0;
}






#include<iostream>
using namespace std;
class TimerManager{
private:
    struct timer{
        int id;
        std::chrono::stready_clock::timer_point expire;
        function<void()>func;
        bool operator<(const timer& other)const{
            return expire<other.expire;
        }
    };
    mutex mtx;
    atomic<int>next_id{0};
    multi_map<std::chrono::stready_clock::timer_point ,timer>timers;
    unordered_map<int,decltype(times)::iterator>active_timers;
public:
    int addTimer(int delay_ms,function<void()>func){
        lock_guard<mutex>lock(mtx);

        int id=next_id++;
        auto now =std::chrono::stready_clock::now();
        auto expire=now+std::chrono::milliseconds(delay_ms);
        auto iter=timers.emplace(expire,timer{id,expire,func});
        active_timers[id]=iter;
    }
    void remove(int id){
        lock_guard<mutex>lock(mtx);
        auto iter=active_timers.find(id);
        if(iter1=active_timers.end()){
            timers.erase(iter.second);
            active_timers.erase(iter);)
        }
    }
    void update(){
       lock_guard<mutex>lock(mtx);
        while(!timers.empty()){
            auto now=std::chrono::stready_clock::now();
            auto iter=timers.begin();
            if(iter.first>now)break;
            if(iter.second.func){
                iter.second.func();
            }
            active.erase(iter.second.id);
            timers.erase(iter);
        }
    }
}
int main(){

    TimerManager tm;
    tm.addTimer(1000, [this]{
        cout<<"hello"<<endl;
    });
    while(1){
        tm.update();
    }
}




#include<iostream>
using namespace std;
class TimerManager{
private:
    struct timer{
        int id;
        std::chrono::stready_clock::timer_point expire;
        function<void()>func;
    };
    mutex mtx;
    multi_map<std::chrono::stready_clock::timer_point expire ,function<void()>func>timers;
    unordered_map<int,decltype<timers>::iterator>active_timers;

public:

    //加入时间函数
    int add_timer(std::chrono::stready_clock::timer_point expire, function<void()>f){
        
    }


    //移除定时任务
    void remove(int id){


    }
    
    //执行定时任务
    void update(){

        
    }


};
int main(){


    while(1){

        update();

    }
    return 0;
}









