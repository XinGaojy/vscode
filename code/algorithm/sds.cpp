#include<iostream>
using namespace std;
struct {
  int len;
  char data[];
}test;
int main(){
  cout<<sizeof(test)<<endl;
  return 0;
}
