#if 0
//三个线程分别打印 A，B，C，要求这三个线程一起运行，打印 n 次，输出形如“ABCABCABC....”的字符串

//两个线程交替打印 0~100 的奇偶数
//通过 N 个线程顺序循环打印从 0 至 100
//多线程按顺序调用，A->B->C，AA 打印 5 次，BB 打印10 次，CC 打印 15 次，重复 10 次
//用两个线程，一个输出字母，一个输出数字，交替输出 1A2B3C4D...26Z

//作者：贾大星
//链接：https://juejin.cn/post/6889233632926384142
//来源：稀土掘金
//著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处


#endif


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




#if 0

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



#endif


#if 0


#include<iostream>
#include<thread>
#include<atomic>
#include<mutex>
#include<condition_variable>
using namespace std;
condition_variable condition;
mutex mtx;
std::atomic<size_t>current_id={0};
void print(int thread_id,char c){
    for(int i=thread_id;i<100*3;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return i==current_id;
        });
        cout<<i<<endl;
        current_id++;
        condition.notify_all();
    }

}
int main(){


    thread t1(print,0,'a');
    
    thread t2(print,1,'a');
    thread t3(print,2,'a');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}


#endif



#if 0

#include<iostream>
#include<thread>
#include<thread>
#include<condition_variable>
#include<mutex>
#include<atomic>

using namespace std;
std::atomic<size_t>current_id={0};
condition_variable condition;
mutex mtx;

void print(int thread_id,char c){
    for(int i=thread_id;i<100*3;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return i==current_id;
                });
        cout<<i<<" "<<c<<endl;
        current_id++;
        
        condition.notify_all();
    }
    
}
int main(){
    thread t1(print,0,'a');
    
    thread t2(print,1,'b');
    thread t3(print,2,'c');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}


#endif






#if 0

#include<iostream>
using namespace std;
mutex mtx;
condition_variable condition;
atomic<size_t>next_id={0};
void print(int thread_id,char c){
    for(int i=thread_id;i<100*3;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{

            return i=next_id;
        });
        cout<<i<<endl;
        next_id++;
        condition.notify_all();
    }
}
int main(){
    thread t1(print,0,'a');
    thread t2(print,1,'b');
    thread t3(print,2,'c');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}



#endif








#include<iostream>
#include<thread>
#include<atomic>
#include<condition_variable>
#include<mutex>
using namespace std;
std::mutex mtx;
std::atomic<int>current_id={0};
condition_variable condition;
void print(int thread_id,char c){
    for(int i=thread_id;i<=100*3;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return current_id==i; 
        });
        current_id++;
        condition.notify_all();
        cout<<i<<endl;
    }
}

int main(){
    thread t1(print,0,'a');
    thread t2(print,1,'b');
    thread t3(print,2,'c');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}


