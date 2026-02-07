#include<iostream>
#include<vector>
using namespace std;

int main(){
  vector<char*>vec;
  while(1){
    vec.push_back(new char[1024*1024*1024*1024]);
  }
  return 0;
}
