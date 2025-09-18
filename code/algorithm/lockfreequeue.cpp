


#include <iostream>
#include <atomic>
#include <vector>
#include<thread>
using namespace std;
template<class T, size_t N = 1024>
class lockfreequeue {
private:
    struct element {
        std::atomic<bool> full_;
        T data_;
    };

    std::atomic<size_t> read_index_;
    std::atomic<size_t> write_index_;
    std::vector<element> data_;

public:
    lockfreequeue() : data_(N) {
        read_index_.store(0);
        write_index_.store(0);
        for (auto& e : data_) {
            e.full_.store(false);
        }
    }

    bool enqueue(T value) {
        size_t write_index = 0;
        element* e = nullptr;
        
        do {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index >= read_index_.load(std::memory_order_acquire) + data_.size()) {
                return false;
            }
            
            size_t index = write_index % data_.size();
            e = &data_[index];
            if (e->full_.load(std::memory_order_relaxed)) {
                return false;
            }
            
        } while (!write_index_.compare_exchange_weak(
            write_index, 
            write_index + 1,
            std::memory_order_release,
            std::memory_order_relaxed));
        
        e->data_ = std::move(value);
        e->full_.store(true, std::memory_order_release);
        return true;
    }

    bool dequeue(T& value) {
        size_t read_index	 = 0;
        element* e = nullptr;
        
        do {
            read_index = read_index_.load(std::memory_order_relaxed);
            if (read_index >= write_index_.load(std::memory_order_acquire)) {
                return false;
            }
            
            size_t index = read_index % data_.size();
            e = &data_[index];
            if (!e->full_.load(std::memory_order_relaxed)) {
                return false;
            }
            
        } while (!read_index_.compare_exchange_weak(
            read_index,
            read_index + 1,
            std::memory_order_release,
            std::memory_order_relaxed));
        
        value = std::move(e->data_);
        e->full_.store(false, std::memory_order_release);
        return true;
    }
};

int main() {
    lockfreequeue<size_t> q;
    std::atomic<size_t> seq(0);
    
    static const int producers = 4;
    static const int consumers = 4;
    static const int iterations = 1000;
    
    // 生产者线程函数
    auto producer = [&]() {
        for (int i = 0; i < iterations; ++i) {
            size_t val = seq.fetch_add(1, std::memory_order_relaxed);
            while (!q.enqueue(val)) {
					cout<<val<<endl;
			
                //std::this_thread::yield();
            }
        }
    };
    
    // 消费者线程函数
    auto consumer = [&]() {
        for (int i = 0; i < iterations; ++i) {
            size_t val;
            while (!q.dequeue(val)) {
					cout<<val<<endl;
			
                //std::this_thread::yield();
            }
            // 处理val...
        }
    };
    
    std::vector<std::thread> producer_threads;
    std::vector<std::thread> consumer_threads;
    
    for (int i = 0; i < producers; ++i) {
        producer_threads.emplace_back(producer);
    }
    
    for (int i = 0; i < consumers; ++i) {
        consumer_threads.emplace_back(consumer);
    }
    
    for (auto& t : producer_threads) t.join();
    for (auto& t : consumer_threads) t.join();
    
    return 0;
}




template<class T>
class lockfreequeue{
private:    
    struct element{
        T data;
        atomic<bool>full_;  
    };
    vector<element>que;
    condition_variable condition;
    mutex mtx;
    atomic<size_t>read_index{0};
    atomic<size_t>write_index{0};
public:
    lockfreequeue(int n):que(n){
        read_index.store(0);
        write_index.store(0);
        for(auto &i:que){
            i.full_=false;
        }
    }
    bool enqueue(T val){
        size_t temp=write_index.load(std::memory_order_acquire);
        do {
            if(temp>=read_index.load(std::memory_order_acquire)+que.size()){
                return false;
            }
            int idx=temp%que.size();
            element& e=que[idx];
            if(e.full_.load(std::memory_order_relaxed))return false;

        }while(!write_index.compare_exchange_weak(temp,temp+1,std::memory_order_release,std::memory_order_relaxed);
            e.data=std::move(val);
            e.full_.store(true,std::memory_order_release);
            return true;
    }
    bool dequeue(T &val){
        size_t temp=read_index.load(std::memory_order_acquire);
        do {
            if(read_index>write_index.load(std::memory_order_acquire)return false;
            int idx=temp%que.size();
            element& e=que[idx];
            if(!e.full_.load(std::memory_order_acquire))return false;
        
        }while(read_index.load(!temp,temp+1,std::memory_order_release,std::memory_order_relaxed);
        val=std::move(e.data_);
        e.full_.store(false,std::memory_order_release);
        return true;
    }
};














#include<iostream>
using namespace std;
template<class T,int N>
class lockfreequeue{
private:
    struct element{
        T data;
        atomic<bool>full_;
    };
    condition_variable condition;
    mutex mtx;
    vector<element>que;
    atomic<size_t>read_index;
    atomic<size_t>write_index;
public:
    lockfreequeue(int n):que(n){
        read_index.store(0);
        write_index.store(0);
        for(auto &i:que){
            i.full_.store(false);
        }
    }
    bool enqueue(int val){
        size_t temp=write_index.load(std::memory_order_acquire);
        do {
                {
                    unique_lock<mutexlock(mtx);
                    if(temp>=read_index.load(std::memory_order_acquire)+que.size())return false;
                    int idx=temp%que.size();
                    element& e=que[idx];
                    if(e.full_.load(std::memory_order_relaxed))return false;

                }
        }while(!write_index.compare_exchange_weak(temp,temp+1,std::memory_order_release,std::memory_order_relaxed));
        
        e.data=std::move(val);
        e.full_.store(std::memory_order_release);
        return true;
    }
    bool dequeue(T & val){
        size_t temp=read_index.load(std::memory_order_acquire);
        do {
            {
                unique_lock<mutex>lock(mtx);
                if(temp>=write_index.load(std::memory_order_acquire))return false;
                size_t index=temp%que.size();
                element &e=que[index];
                if(!e.full_.load(std::memory_order_relaxed))return false;

            }

        }while(!read_index.compare_exchange_weak(temp,temp+1,std::memory_order_release,std::memory_order_relaxed));
        val=e.data;
        e.full_.store(false,std::memory_order_release);
        return true;
    }
};



int main(){

    return 0;
}









class lockfreequeue{
private:
    struct element{
        T data_;
        atomic<bool>full_;
    };
    vector<elment>que;
    size_t read_index;
    size_t write_index;
    mutex mtx;
    condition_variable condition;
public:
    bool enqueue(T val){
        size_t temp=write_index.load(std::memory_order_acquire);
        do {
            if(temp>=read_index.load(std::memory_order_acquire)+que.size())return false;
            size_t index=temp%que.size();
            element& e=que[index];
            if(e.full_.load(std::memory_order_acquire))return false;
        }while(!write_index.compare_exchange_weak(temp,temp+1,std::memory_order_relase,std::memory_order_relaxed));
        e.data=std::move(val);
        e.full_.store(true,std::memory_order_release);
        return true;
    }
}





#include<iostream>
using namespace std;
template<class T>
class lockfreequeue{
private:
    struct element {
        T data_;
        atomic<bool>full_;
    };
    vector<element>que;
    // size_t capacity_;
    // size_t size_;
    atomic<size_t>read_index_;
    atomic<size_t>write_index_;

    
public:
    lockfreequeue(int n):vec(n){
        for(auto &e:que){
            e.full_.store(false);
        }
        read_index_.store(0);
        write_index_.store(0);
    }
    bool enqueue(T &val){
        size_t temp=write_index_.load(std::memory_order_acquire);
        do {
            if(temp>read_index_.load(std::memory_order_acquire)+que.size())return false;
            size_t  index=(temp)%que.size();
            element&  e=que[index];
            if(e.full_.load(std::memory_order_relaxed))return false;

        }  while(!write_index.store(temp,
                                    temp+1,
                                    std::memory_order_relase,
                                    std::memory_order_relaxed));

        e.data_.store(std::move(val),std::memory_order_release);
        e.full_.store(true,std::memory_order_release);
        return true;
        

    }
    bool dequeue(T &val){
        size_t temp=read_index.load(std::memory_order_acquire);
        if(temp>=write_index_.load(std::memory_order_acquire))return false;
        int idex=temp%que.size();
        element& e=que[idex];
        
    }

};
int main(){
    return 0;
}







