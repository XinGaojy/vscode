#include <iostream>
#include <vector>
using namespace std;
#if 0
void quicksort(vector<int>& vec,int left,int right){
  if(left>=right){
    return ;
  }

  int i=left-1;
  int j=right+1;
  int mid=(right-left)/2+left;
  int x=vec[mid];
  while(i<j){
    do i++;while(vec[i]<x);
    do j--;while(vec[j]>x);
    if(i<j){
      swap(vec[i],vec[j]);
    }
  }
  quicksort(vec,left,j);
  quicksort(vec,j+1,right);
}

#endif

#if 0
vector<int>res;
int quicksort(vector<int>& vec,int left,int right,int k){

  if(left>=right){
    return vec[left];  
  }

  int i=left-1;
  int j=right+1;
  int mid=(right-left)/2+left;
  int x=vec[mid];
  while(i<j){
    do i++;while(vec[i]<x);
    do j--;while(vec[j]>x);
    if(i<j){
      swap(vec[i],vec[j]);
    }
  }
  if(k<=j)return res.push_back(vec[i]),quicksort(vec,left,j,k);
  else return quicksort(vec,j+1,right,k);

}

#endif

#if 0

// 快速选择算法，返回前k大元素的起始位置
int quickSelect(vector<int>& vec, int left, int right, int k) {
    if (left == right) return left;
    
    int i = left - 1;
    int j = right + 1;
    int mid = (right - left) / 2 + left;
    int pivot = vec[mid];
    
    // 降序分区：将大于pivot的放在左边，小于pivot的放在右边
    while (i < j) {
        do i++; while (vec[i] > pivot);  // 注意：这里是 > 而不是 <
        do j--; while (vec[j] < pivot);  // 注意：这里是 < 而不是 >
        if (i < j) {
            swap(vec[i], vec[j]);
        }
    }
    
    // 现在j是分区点的位置
    int leftSize = j - left + 1;
    
    if (leftSize >= k) {
        // 前k大元素在左半部分
        return quickSelect(vec, left, j, k);
    } else {
        // 前k大元素跨越左右两部分
        return quickSelect(vec, j + 1, right, k - leftSize);
    }
}

int main() {
    vector<int> vec = {1, 4, 3, 2, 2, 3, 5, 2, 1};
    int k = 3;
        
    quickSelect(vec, 0, vec.size() - 1, k);
   for (auto i : vec) {
        cout << i << " ";
    }
   
    return 0;
}

#endif

#if 0

int main(){
  vector<int>vec={1,4,3,2,2,3,5,2,1};
   cout<< quicksort(vec,0,vec.size()-1,3);
   // quicksort(vec,0,vec.size()-1);
#if 0
  for(auto i:vec){
    cout<<i<<endl;
  }

#endif
  
  for(auto i:res){
    cout<< i<< endl;
  }
  
  return 0;
}

#endif

#if 1

#include <iostream>
#include <vector>
using namespace std;
void quicksort(vector<string>& vec, int left, int right) {
  if (left >= right) return;
  int i = left - 1;
  int j = right + 1;
  int mid = (right - left) / 2 + left;
  string x = vec[mid];
  while (i < j) {
    do i++;
    while (vec[i] < x);
    do j--;
    while (vec[j] > x);
    if (i < j) {
      swap(vec[i], vec[j]);
    }
  }
  quicksort(vec, left, j);
  quicksort(vec, j + 1, right);
}

int main() {
  vector<string> vec = {"abc", "ab", "ac", "db"};
  int k = 0;
  quicksort(vec, 0, vec.size() - 1);
  for (auto i : vec) {
    cout << i << endl;
  }
  return 0;
}

#endif
#include<iostream>
using namespace std;
void quicksort(vector<int>&vec,int left,int right){
  if(left>=right)return;
  int mid=(right -left)/2+left;
  int mid=(right-left)/2+left;
  int x=vec[mid];
  while(i<j){
    do i++;while(vec[i]<x);
    do j--;while(vec[j]>x);
    if(i<j)swap(vec[i],vec[j]);
  }
  quicksort(vec,left,j);
  quicksort(vec,j+1,right);
}
int main(){
  vector<int>vec={1,4,3,2};
  quicksort(vec,0,vec.size()-1);
  for(auto i:vec){
    cout<<i<<endl;
  }
  return 0;
}




