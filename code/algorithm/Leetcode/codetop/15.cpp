
//15.三数之和
//
//
#include<algorithm>
#include<iostream>
#include<vector>
using namespace std;
vector<vector<int>> Threenumssum(vector<int>& nums,int target){
    sort(nums.begin(),nums.end());
    vector<vector<int>>res;
    for(int i=0;i<nums.size();i++){
      if(i>0&& nums[i]==nums[i-1])continue;
      int j=i+1;
      int k=nums.size()-1;
      while(j<k){
        int sum=nums[i]+nums[j]+nums[k];
        if(sum>target)k--;
        else if(sum<target)j++;
        else {
              res.push_back({nums[i],nums[j],nums[k]});
              j++;
              while(j<k&& nums[j-1]==nums[j])j++;
              k--;
              while(j<k&& nums[k+1]==nums[k])k--;
            }
      }

    }
    return res;
}
int main(){
  vector<int>vec={1,6,5,4,3,2};
  int target=6;
  vector<vector<int>>res=Threenumssum(vec,target);
  for(auto i:res){
    for(auto j:i){
      cout<<j<<" ";
    }
    cout<<endl;
  }
  return 0;
}
