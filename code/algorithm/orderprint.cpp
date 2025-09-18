

#if 0
#include<iostream>
#include<thread>
#include<mutex>
//#include<mutex>
#include<condition_variable>
#include<atomic>
using namespace std;
mutex mtx;
condition_variable condition;
atomic<int>current_id{0};
void print(int thread_id,char c,int maxvalue){
    for(int i=thread_id;i<3*maxvalue;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return i==current_id;
        });
        cout<<"thread_id"<<thread_id<<" "<<c<<" "<<"current_id"<<" "<<current_id<<endl;
        current_id++;
        condition.notify_all();
    }
}
int main(){
    const int maxvalue=100;
    thread t1(print,0,'a',maxvalue);
    thread t2(print,1,'b',maxvalue);
    thread t3(print,2,'c',maxvalue);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}

#endif


#if 0
// 多个线程顺序打印
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

using namespace std;

condition_variable condition;
mutex mtx;
atomic<int> current_num(0);

void printnum(int thread_id, int max_num) {
    for (int i = thread_id; i < max_num; i += 4) {  // 修改循环条件
        unique_lock<mutex> lock(mtx);
        condition.wait(lock, [i] {  // 修复lambda捕获
            return current_num == i;
        });
        
        cout << "Thread " << thread_id << ": " << i << endl;
        current_num++;
        
        condition.notify_all();
    }
}

int main() {
    const int max_num = 100;
    
    thread t1(printnum, 0, max_num);
    thread t2(printnum, 1, max_num);
    thread t3(printnum, 2, max_num);
    thread t4(printnum, 3, max_num);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    return 0;
}
#endif






//实现顺序打印


#if 0
#include<iostream>
#include<mutex>
#include<thread>
#include<atomic>
#include<condition_variable>
using namespace std;
condition_variable condition;
mutex mtx;
atomic<int>current_id{0};
void print(int thread_id,int maxvalue,char c){
    for(int i=thread_id;i<3*maxvalue;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return current_id==i;
        });
        cout<<i<<endl;
        current_id++;
        condition.notify_all();
    }
}
int main(){
    const int maxvalue=100;
    thread t1(print,0,maxvalue,'a');
    thread t2(print,1,maxvalue,'b');
    thread t3(print,2,maxvalue,'c');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}

#endif






#include<iostream>
#include<atomic>
#include<thread>
#include<condition_variable>
using namespace std;
mutex mtx;
condition_variable condition;
atomic<int>current_id{0};
void print(int thread_id,int maxvalue,char c){
    for(int i=thread_id;i<3*maxvalue;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return i==current_id;
        });
        cout<<thread_id<<" "<<i<<" "<<current_id<<" "<<c<<endl;
        current_id++;
        condition.notify_all();
    }
}
int main(){
    const int maxvalue=100;
    thread t1(print,0,maxvalue,'a');
    thread t2(print,1,maxvalue,'b');
    thread t3(print,2,maxvalue,'c');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}










