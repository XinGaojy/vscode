

#if 1

#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <iostream>
#include <chrono>

using namespace std;

template <class T>
class BlockQueue {
private:
    std::mutex mtx_;                  // 互斥锁，保护共享数据
    std::condition_variable not_empty_; // 非空条件变量
    std::condition_variable not_full_;  // 非满条件变量
    const size_t capacity_;            // 队列容量
    vector<T> buffer_;                 // 循环缓冲区
    size_t size_ = 0;                  // 当前元素数量
    size_t head_ = 0;                  // 队列头指针
    size_t tail_ = 0;                  // 队列尾指针

public:
    explicit BlockQueue(size_t capacity)
        : capacity_(capacity),
          buffer_(capacity_),
          size_(0),
          head_(0),
          tail_(0) {}

    // 阻塞式入队
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待队列不满（条件变量+谓词防止虚假唤醒）
        not_full_.wait(lock, [this] {
            return size_ < capacity_;
        });

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
        size_++;

        // 通知一个等待的消费者
        not_empty_.notify_one();
        return true;
    }

    // 非阻塞式入队
    bool tryPush(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (size_ >= capacity_) return false;

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;
        size_++;

        not_empty_.notify_one();
        return true;
    }

    // 阻塞式出队
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待队列不空
        not_empty_.wait(lock, [this] {
            return size_ > 0;  // 修正：原代码 tail_ > head_
            在循环队列中不准确
        });

        item = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        size_--;

        // 通知一个等待的生产者
        not_full_.notify_one();
        return true;
    }

    // 非阻塞式出队
    bool tryPop(T& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (size_ == 0) return false;

        item = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        size_--;

        not_full_.notify_one();
        return true;
    }

    // 获取当前元素数量（线程安全）
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return size_;
    }

    // 判断是否为空（线程安全）
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return size_ == 0;
    }

    // 判断是否已满（线程安全）
    bool full() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return size_ == capacity_;
    }

    // 随机访问（线程不安全，需外部同步）
    const T& at(size_t index) const {
        if (index >= size_) {  // 修正：原代码 tail_ 判断不准确
            throw std::out_of_range("Index out of range");
        }
        return buffer_[(head_ + index) % capacity_];
    }
};

int main() {
    BlockQueue<int> que(5);

#if 1
    // 生产者线程
    std::thread producer([&]() {
        for (int i = 0; i < 5; i++) {
            que.push(i);
            std::cout << "Produced: " << i << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // 消费者线程
    std::thread consumer([&]() {
        for (int i = 0; i < 3; i++) {
            int item = 0;
            que.pop(item);
            std::cout << "Consumed: " << item << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // 主线程消费
    int item = 0;
    que.pop(item);
    cout << "Main consumed: " << item << endl;

    producer.join();
    consumer.join();

#endif
    return 0;
}
#endif

#if 0
class BlockQueue{
private:
    int size_;
    int capacity_;
    int head_;
    int tail_;
    vector<int>que;
    condition_variable not_full_;
    condition_variable not_empty_;
    mutex mtx;
public:
    BlockQueue(int n):que(n),capacity_(n){
        size_=0;
        head_=0;
        tail_=0;
    }
    bool enqueue(int val){
        unique_lock<mutex>lock(mtx);
        not_full_.wait(lock,[this]{
            return size_<=capacity_;
        });
        que[tail_]=std::move(val);
        tail_=(tail_+1)%capacity_;
        size_++;
        not_empty_.notify_one();
        return true;
    }
    bool dequeue(int &val){
        unique_lock<mutex>lock(mtx);
        not_empty_.wait(lock,[this]{
            return size_>0;
        });
        val=std::move(que[head_]);
        head_=(head_+1)%capacity_;
        size_--;
        not_full_.notify_one();
        return true;
    }
};

#endif

#if 0
#include <iostream>
using namespace std;
class BlockQueue{
private:
    int size_;
    int capacity_;
    vector<int>que;
    int head_;
    int tail_;
    mutex mtx;
    condition_variable not_full_;
    condition_variable not_empty_;
public:
    BlockQueue(int n):capacity(n),que(n){
        size_=0;
        head_=0;
        tail_=0;
    }
    bool enqueue(int val){
        {
            unique_lock<mutex>lock(mtx);
            not_full_.wait(lock,[this]{
                return size_<capacity_;
            });
            vec[head_]=std::move(val);
            tail_=(tail_+1)%capacity_;
            size_++;

        }
        not_empty_.notify_one();
        return true;
    }
    bool dequeue(int &val){
        {
            unique_lock<mutex>lock(mtx);
            not_empty_.wait(lock,[this]{
                return size_>0;
            });
            val=std::move(vec[head_]);
            head_=(head_+1)%capacity_;
            size_--;
        }
        not_full_.notify_one();
        return true;
    }
};
int main(){

    return 0;
}

#endif

class BlockQueue{
private:
    int size_;
    int capacity_;
    mutex mtx;
    condition_variable not_empty_;
    condition_variable not_full_;
    vector<int>que;
    int head_;
    int tai_;
public:
    BlockQueue(int n):capacity(n),que(n){
        size_=0;
        capacity_=0;
        head_=0;
        tail_=0;
    }
    bool enqueue(int val){
        {
            unique_lock<mutex>lock(mtx);
            not_full_.wait(lock.[this]{
                return size_<capacity_;
            });
            que[tail_]=val;
            tail_=(tail_+1)%capacity_;
            size_++;
            not_empty_.notify_one();
        }
        return true;
    }
    bool dequeue(int& val){
        {
            unique_lock<mutex>lock(mtx);
            not_empty_.wait(lock,[this]{
                return size_>0;
            });
            val=que[head_];
            head_=(head_+1)%capacity_;
            size_--;
            not_full_.notify_one();

        }
        return true;
    }
}




