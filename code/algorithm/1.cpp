#include<iostream>
using namespace std;

class Threadpool
{
private:
  struct task{
    int id;
    function<void()>func;
  };
  vector<thread>threads;
  variable_condition condition;
  mutex mtx;
  bool stop;
  atomic<int>current_id={0};
  atomic<int>next_id={0};
  deque<task>tasks;
public:
  Threadpool(int n):stop(false) {
    for(int i=0; i<n; i++){
      threads.emplace_back([this]{
          while(1){
            task tk={-1,nullptr};
            {
              lock_guard<mutex>lock(mtx);
              condition.wait([this]{
                  return stop|| !tasks.empty() && current_id!=tasks.front().id;
              
              });
              if(stop&& tasks.empty()){
                return ;
              }
              tk=tasks.front();
              tasks.pop_front();
            }
            tasuk

          }
      });
    }
  }
};
int main(){

  return 0;
}
