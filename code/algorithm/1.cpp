
#if 0
#include <iostream>
using namespace std;

class Threadpool {
 private:
  struct task {
    int id;
    function<void()> func;
  };
  vector<thread> threads;
  variable_condition condition;
  mutex mtx;
  bool stop;
  atomic<int> current_id = {0};
  atomic<int> next_id = {0};
  queue<task> tasks;

 public:
  Threadpool(int n) : stop(false) {
    for (int i = 0; i < n; i++) {
      threads.emplace_back([this] {
        while (1) {
          task tk = {-1, nullptr};
          {
            lock_guard<mutex> lock(mtx);
            condition.wait([this] {
              return stop || !tasks.empty() && current_id != tasks.front().id;
            });
            if (stop && tasks.empty()) {
              return;
            }
            tk = tasks.front();
            tasks.pop_front();
          }
          tasuk
        }
      });
    }
  }
};
int main() { return 0; }

#endif

#if 0

#include <iostream>
#include <vector>
using namespace std;
int quicksort(vector <int>& vec, int left, int right, int k) {
  if (left >= right) return vec[left];
  int i = left - 1;
  int j = right + 1;
  int mid = (right - left) / 2 + left;
  int x = vec[mid];
  while (i < j) {
    do i++;
    while (vec[i] < x);
    do j--;
    while (vec[j] > x);
    if (i < j) swap(vec[i], vec[j]);
  }
  if (k <= j)
    return quicksort(vec, left, j, k);
  else
    return quicksort(vec, j + 1, right, k);
}
int main() {
  int k = 2;
  vector<int> vec = {1, 5, 4, 3, 2};
  int x = quicksort(vec, 0, vec.size(), k);
  cout << x << endl;

  return 0;
}

#endif

#include<iostream>
using namespace std;
int main(){
  cout<<35*15<<endl;
  cout<<33*15<<endl;
  cout<<30*15<<endl;
  cout<<26*16<<endl; 
  return 0;
}

