//模拟并且解决死锁
//

#if 0
#include<iostream>
#include<chrono>
#include<thread>
#include<mutex>
#include<unistd.h>
using namespace std;
std::mutex mtx1;
std::mutex mtx2;
void thread1(int thread_id){
  lock_guard<mutex>lock1(mtx1);
  sleep(1);
  lock_guard<mutex>lock2(mtx2);
}

void thread2(int thread_id){
  lock_guard<mutex>lock2(mtx2);
  sleep(1);
  lock_guard<mutex>lock1(mtx1);
}


int main(){
  thread t1(thread1,0);
  thread t2(thread2,0);
  t1.join();
  t2.join();
  return 0;
}




#endif





#if 0
#include<iostream>
#include<mutex>
#include<unistd.h>
#include<thread>
using namespace std;
mutex mtx1;
mutex mtx2;
mutex mtx3;
void thread1(){
  lock_guard<mutex>lock1(mtx1);
  sleep(1);
  lock_guard<mutex>lock2(mtx2);

}

void thread2(){
  lock_guard<mutex>lock2(mtx2);
  sleep(1);
  lock_guard<mutex>lock3(mtx3);

}
void thread3(){
  lock_guard<mutex>lock3(mtx3);
  sleep(1);
  lock_guard<mutex>lock1(mtx1);
}
int main(){
  thread t1(thread1);
  thread t2(thread2);
  thread t3(thread3);
  t1.join();
  t2.join();
  t3.join();
  return 0;
}

#endif



#include<mutex>
#include<thread>
#include<iostream>
#include<unistd.h>
using namespace std;
mutex mtx1,mtx2,mtx3;
void f1(){
  lock_guard<mutex>lock1(mtx1);
  sleep(1);
  lock_guard<mutex>lock2(mtx2);
}

void f2(){
  lock_guard<mutex>lock2(mtx2);
  sleep(1);
  lock_guard<mutex>lock3(mtx3);
}

void f3(){
  lock_guard<mutex>lock3(mtx3);
  sleep(1);
  lock_guard<mutex>lock1(mtx1);
}
int main(){
  thread t1(f1);

  thread t2(f2);
  thread t3(f3);
  t1.join();
  t2.join();
  t3.join();  
}
