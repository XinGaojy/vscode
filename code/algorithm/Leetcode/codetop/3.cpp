#include<iostream>
#include<unordered_map>
#include<string>
using namespace std;
string maxlen(string s){
  unordered_map<char,int>map;
  int left=0;
  int index=0;
  int res=0;
  for(int i=0;i<s.size();i++){
    map[s[i]]++;
    while(map[s[i]]>1){
      map[s[left++]]--;
      
    }
    if(res<i-left+1){
      res=i-left+1;
      index=left;
    }
  }
  string s1=s.substr(index,res);
  return s1;
}
int main(){
  string s="adfjdfa";
  string s1=maxlen(s);
  cout<<s1<<endl;
  return 0;
}
