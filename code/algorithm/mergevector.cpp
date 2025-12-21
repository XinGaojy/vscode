//合并两个数组并去重
//
//
//

#if 0
#include<iostream>
#include<vector>
using namespace std;
void mergevector(vector<int>vec1,vector<int>vec2,vector<int>&res){
  int left=0;
  int right=0;
  while(left<vec1.size() && right<vec2.size()){
    if(vec1[left]<vec2[right]){
      if(res.empty() || res.back()!=vec1[left]){
        res.push_back(vec1[left]);
      }
      left++;
    }else if(vec1[left]==vec2[right]){
      if(res.empty() || res.back()!=vec1[left]){
        res.push_back(vec1[left]);
      }
      left++;
      right++;
    }else{
      if(res.empty() || res.back()!=vec2[right]){
        res.push_back(vec2[right]);
      }
      right++;
    }
  }

  while(left<vec1.size()){
    if(res.empty() || res.back()!=vec1[left]){
      res.push_back(vec1[left]);
    } 
    left++;
  }
  while(right<vec2.size()){
    if(res.empty() || res.back()!=vec2[right]){
      res.push_back(vec2[right]);

    }
    right++;
  } 
}
int main(){
  vector<int>vec1={1,1,1,2,2,2,3,4,4,5};
  vector<int>vec2={1,1,2,3,3,3,4,4};
  vector<int>res;
  mergevector(vec1,vec2,res);
  for(auto i:res){
    cout<<i<<endl;
  }


  return 0;
}


#endif

#if 0

#include <iostream>
#include <vector>
using namespace std;

void mergeVector(const vector<int>& vec1,
                 const vector<int>& vec2,
                 vector<int>& res)
{
    size_t left = 0, right = 0;
    res.clear();
    res.reserve(vec1.size() + vec2.size());

    /* 双指针合并 + 去重 */
    while (left < vec1.size() && right < vec2.size()) {
        if (vec1[left] < vec2[right]) {
            if (res.empty() || res.back() != vec1[left])
                res.push_back(vec1[left]);
            ++left;
        }
        else if (vec1[left] == vec2[right]) {
            if (res.empty() || res.back() != vec1[left])
                res.push_back(vec1[left]);
            ++left;
            ++right;
        }
        else {  // vec1[left] > vec2[right]
            if (res.empty() || res.back() != vec2[right])
                res.push_back(vec2[right]);
            ++right;
        }
    }

    /* 把剩余元素直接尾插（保证有序且不与最后元素重复） */
    while (left < vec1.size()) {
        if (res.empty() || res.back() != vec1[left])
            res.push_back(vec1[left]);
        ++left;
    }
    while (right < vec2.size()) {
        if (res.empty() || res.back() != vec2[right])
            res.push_back(vec2[right]);
        ++right;
    }
}

int main() {
    vector<int> vec1 = {1, 1, 2, 2, 2, 3, 4, 4, 5};
    vector<int> vec2 = {1, 1, 2, 3, 3, 3, 4, 4};
    vector<int> res;
    mergeVector(vec1, vec2, res);

    for (int x : res) cout << x << '\n';
    return 0;
}


#endif





#include<iostream>
#include<vector>
using namespace std;
void mergeVector(vector<int>vec1,vector<int>vec2,vector<int>&res){
  int left=0;
  int right=0;
  while(left<vec1.size()&& right<vec2.size()){
    if(vec1[left]<vec2[right]){
      if(res.empty() || res.back()!=vec1[left]){
          res.push_back(vec1[left]);
      }
      left++;
    }else if(vec1[left]==vec2[right]){
      if(res.empty() || res.back()!=vec2[right]){
        res.push_back(vec2[right]);
      }
      left++;
      right++;
    }else{
      if(res.empty() || res.back()!=vec2[right]){
        res.push_back(vec2[right]);
      }
      right++;
    }
  }

  while(left<vec1.size()){
    if(res.empty() || res.back()!=vec1[left]){
      res.push_back(vec1[left]);
    }
    left++;
  }
  while(right<vec2.size()){
    if(res.empty() || res.back() !=vec2[right]){
      res.push_back(vec2[right]);
    }
    right++;
  }
}
int main(){
    vector<int> vec1 = {1, 1, 2, 2, 2, 3, 4, 4, 5};
    vector<int> vec2 = {1,1, 1, 2, 3, 3, 3, 4, 4};
    vector<int> res;
    mergeVector(vec1, vec2, res);
    for(auto i:res){
      cout<<i<<endl;
    }
  return 0;
}
