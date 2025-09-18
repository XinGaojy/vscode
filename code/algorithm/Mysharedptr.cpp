

#if 1
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <stdexcept>

using namespace std;

template<class T>
class shared_ptr {
private:
    std::atomic<std::size_t>* ref_count;
    T* ptr;

    void release() {
        if (ref_count && ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr;
            delete ref_count;
        }
    }

public:
    shared_ptr() : ptr(nullptr), ref_count(nullptr) {}

    explicit shared_ptr(T* p) : ptr(p), ref_count(p ? new std::atomic<std::size_t>(1) : nullptr) {}

    ~shared_ptr() {
        release();
    }

    shared_ptr(const shared_ptr<T>& other) {
        ptr = other.ptr;
        ref_count = other.ref_count;
        if (ref_count) {
            ref_count->fetch_add(1, std::memory_order_relaxed);
        }
    }

    shared_ptr& operator=(const shared_ptr<T>& other) {
        if (&other != this) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            if (ref_count) {
                ref_count->fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    shared_ptr(shared_ptr<T>&& other) noexcept {
        ptr = other.ptr;
        ref_count = other.ref_count;
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }

    shared_ptr<T>& operator=(shared_ptr<T>&& other) {
        if (&other != this) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            other.ref_count = nullptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

    T* get() const {
        return ptr;
    }

    void reset(T* p = nullptr) {
        release();
        ptr = p;
        ref_count = p ? new std::atomic<std::size_t>(1) : nullptr;
    }
    int get_count(){
        return *ref_count;
    }
};

void test_shared_ptr_thread_safety() {
    shared_ptr<int> ptr(new int(32));
    const int num_threads = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&ptr] {
            for (int j = 0; j < 10; j++) {
                shared_ptr<int> local_ptr(ptr);
                *local_ptr = *local_ptr + 1;
                cout << *local_ptr << endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {




    int *p=new int(1);
    shared_ptr<int> ptr1(new int(10));
    cout<<ptr1.get_count();
    shared_ptr<int> ptr2 = ptr1;
    shared_ptr<int>ptr3(p);
    ptr2=std::move(ptr3);
    cout << *ptr2;
    cout<<ptr2.get_count();
    //test_shared_ptr_thread_safety();
    return 0;
}
#endif






// template<class T>
// class Mysharedptr{
// private:
//     T*ptr;
//     atomic<size_t>* ref_count;
//     void release(){
//         if(ref_count&&ref_count->fetch_sub(1,std::memory_order_acq_rel)==1){
//             delete ptr;
//             delete ref_count;
//         }

//     }
// public:
//     explicit Mysharedptr():ref_count(nullptr),ptr(nullptr){}
//     ~Mysharedptr(){
//         release();
//     }
//     Mysharedptr(T*p):ptr(p),ref_count(p?new atomic<size_t>(1):nullpr){}
//     Mysharedptr(const Mysharedptr<T>&other){
//         ref_count=other.ref_count;
//         ptr=other.ptr;
//         if(ref_count){
//             ref_count->fetch_add(1,std::memory_order_relaxed);
//         }
//     }
//     Mysharedptr(Mysharedptr<T>&&other){
//         ref_count=other.ref_count;
//         ptr=other.ptr;
//         other.ref_count=nullptr;
//         other.ptr=nullptr;
//     }
//     Mysharedptr<T>& operator=(const Mysharedptr<T>& other){
//         if(&other!=this){
//             release();
//             ref_count=other.ref_count;
//             ptr=other.ptr;
//             if(ref_count){
//                 ref_count->fetch_add(1,std::memory_order_relaxed);
//             }
//         }
//         return *this;
//     }
//     Mysharedptr<T>& operator=(Mysharedptr<T>&& other){
//         if(&other!=this){
//             release();
//             ref_count=other.ref_count;
//             ptr=other.ptr;
//             other.ref_count=nullpr;
//             other.ptr=nullptr;
//         }
//         return *this;
//     }
//     void reset(T*p){
//         release();
//         ptr=p;
//         ref_count=p?new atomic<size_t>(1):nullptr;
//     }
// }









#if 0
#include<iostream>
using namespace std;
template<class T>
class Mysharedptr{
private:
    atomic<T>*ref_count;
    T *ptr;
    void release(){
        if(ref_count&&ref_count.fetch_sub(1,std::memory_order_acq_rel)==1){
            delete ptr;
            delete ref_count;
        }
    }

public:
    Mysharedptr(){
        ref_count=nullptr;
        ptr=nullptr;

    }
    ~Mysharedptr(){
        release();
    }
    explicit Mysharedptr(const void* p){
        if(p){
            ptr=p;
            ref_count(p?new atomic<T>(1):nullptr);
        }
    }
    Mysharedptr(const Mysharedptr&other )
    {
        ptr=other.ptr;
        ref_count=other.ref_count;
        if(ref_count){
            ref_count.fetch_add(1,std::memory_order_release);
        }
    }
    Mysharedptr(Mysharedpt&& other)noexcept{
        ptr=other.ptr;
        ref_count=other.ref_count;
        other.ptr=nullptr;
        other.ref_count=nullptr;
    }
    Mysharedptr& operator=(const Mysharedptr& other){
        if(&other!=this){
            release();
            ptr=other.ptr;
            ref_count=other.ref_count;
            ref_count.fetch_add(1,std::memory_order_release);
        }
        return *this;
    }
    Mysharedptr& operator=(Mysharedptr&&other) noexcept{
        if(&other!=this){
            release();
            ptr=other.ptr;
            ref_count=other.ref_count;
            ptr=nullptr;
            ref_count=nullptr;
        }
        return *this;
    }
    void reset(T *p){
        release();
        ptr=p;
        ref_count(p?new atomic<T>(1):nullptr);
    }
}
int main(){
    return 0;
}
#endif




