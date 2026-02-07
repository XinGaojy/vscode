//合并两个数组并去重
//
//
//

#if 0
#include <iostream>
#include <vector>
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

#if 0

#include <iostream>
#include <vector>
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

#endif

#if 0

#include <iostream>
#include <vector>
using namespace std;
vector<int> mergevector(vector<int> vec1, vector<int> vec2) {
  int left = 0;
  int right = 0;
  vector<int>res;
  while (left < vec1.size() && right < vec2.size()) {
    if (vec1[left] < vec2[right]) {
      if (res.empty() || res.back() != vec1[left]) {
        res.push_back(vec1[left]);
      }
      left++;

    } else if (vec1[left] == vec2[right]) {
      if (res.empty() || res.back() != vec1[left]) {
        res.push_back(vec1[left]);
      }
      left++;
      right++;

    } else {
      if (res.empty() || res.back() != vec2[right]) {
        res.push_back(vec2[right]);
      }
      right++;
    }
  }

  while (left < vec1.size()) {
    if (res.empty() || res.back() != vec1[left]) {
      res.push_back(vec1[left]);
    }
    left++;
  }

  while (right < vec2.size()) {
    if (res.empty() || res.back() != vec2[right]) {
      res.push_back(vec2[right]);
    }
    right++;
  }

  return res;
}

vector<int> merge(vector<vector<int>>vec,int left,int right){
  if(left>=right)return vec[left];
  int mid=(right-left)/2+left;
  vector<int>vec1=merge(vec,left,mid);
  vector<int>vec2=merge(vec,mid+1,right);
  return mergevector(vec1,vec2);
}

int main() {
  vector<int> vec1 = {1, 1, 2, 3, 3, 3, 5};
  vector<int> vec2 = {1, 1, 1, 1, 2, 2, 2, 2};
  vector<int>vec3={1,1,1,1,2,2,2,3,3,3};
  vector<vector<int>>vec;
  vec.push_back(vec1);
  vec.push_back(vec2);
  vec.push_back(vec3);
  vector<int> res= merge(vec, 0, vec.size()-1);
  for (auto i : res) {
    cout << i << endl;
  }
  return 0;
}

#endif

//将两个链表合并并去重

#include <iostream>
#include <vector>
using namespace std;

struct ListNode {
  int value;
  ListNode* next;
  ListNode(int v) : value(v), next(nullptr) {}
};

#if 0
ListNode*  mergeList(ListNode* head1,ListNode* head2){
    ListNode* pre=head1;
    ListNode* cur=head2;
    ListNode* dummy=new ListNode(-1);
    ListNode* p=dummy;
    while(pre && cur){
        if(pre->value<cur->value){
            p->next=pre;
            pre=pre->next;
        }else{
            p->next=cur;
            cur=cur->next;
        }
        p=p->next;
    }
    p->next=pre ? pre:cur;
    return dummy->next;
}

#endif

#if 1
ListNode* mergeList(ListNode* head1, ListNode* head2) {
  ListNode* pre = head1;
  ListNode* cur = head2;
  ListNode* dummy = new ListNode(-1);
  ListNode* p = dummy;
  while (pre && cur) {
    if (pre->value < cur->value) {
      if (p->value != pre->value) {
        p->next = pre;
        p = p->next;
      }
      pre = pre->next;
    } else if (pre->value == cur->value) {
      if (p->value != pre->value) {
        p->next = pre;
        p = p->next;
      }
      pre = pre->next;
      cur = cur->next;
    } else {
      if (p->value != cur->value) {
        p->next = pre;
        p = p->next;
      }
      cur = cur->next;
    }
  }

  while (pre) {
    if (p->value != pre->value) {
      p->next = pre;
      p = p->next;
    }
    pre = pre->next;
  }

  while (cur) {
    if (p->value != cur->value) {
      p->next = cur;
      p = p->next;
    }
    cur = cur->next;
  }
  return dummy->next;
}

#endif

ListNode* createList(vector<int>& vec) {
  ListNode* dummy = new ListNode(-1);
  ListNode* p = dummy;
  for (auto i : vec) {
    p->next = new ListNode(i);
    p = p->next;
  }
  return dummy->next;
}

void print(ListNode* head) {
  ListNode* cur = head;
  while (cur) {
    cout << cur->value << endl;
    cur = cur->next;
  }
}

int main() {
  vector<int> vec1 = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 6, 6, 6, 7};
  vector<int> vec2 = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9};
  ListNode* head1 = createList(vec1);
  ListNode* head2 = createList(vec2);
  ListNode* head = mergeList(head1, head2);
  print(head);
  return 0;
}
endif



