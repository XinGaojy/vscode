#include<iostream>
using namespace std;
int  maxlenstring(string s){
  int len=1;
  int count=1;

  for(int i=1;i<s.size();i++){
    if(s[i]==s[i-1]){
      count++;  
      len=max(len,count);
    }
    else {
      count=1;
    }
  }
  return len;
}
int main(){
  string s="11111132222222222222";
  int count=maxlenstring(s);
  cout<<count<<endl;
  return 0;
}
