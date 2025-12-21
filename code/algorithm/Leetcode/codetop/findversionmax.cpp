
#include<iostream>
using namespace std;

int comparestring(string s,string t){
  int left=0;
  int right=0;
  int res1=0;
  int res2=0;

  while(left<s.size() ||right<t.size()){
        
      while(left<s.size()&& s[left]!='.'){
        res1=res1*10+s[left];
        left++; 

      }

      while(right<s.size() && s[right]!='.'){
        res2=res2*10+s[right];
        right++;
        }

      if(res1<res2){
        return 1;
      }
      else if(res1>res2){
        return -1;
      }
      else{
        left++;
        right++;
      }
  

          }
          }
string findversionmax(vector<string>&vec){
  string s;
  for(auto i:vec){
      if(comparestring(s,i)){
      
        s=i;
      }
  }  
  return s;
}


int main(){
  vector<string>vec={"1.1.1.2","2.2.3.4"};
  findversionmax(vec);
  return 0;
}
