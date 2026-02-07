#include<iostream>
using namespace std;
template <typename T>
T add(T a, T b) {
  return a + b;
}
int main() {
  int i = add(1, 2);
  double d = add(1.1, 2.2);
  cout<<i<<endl;
  cout<<d<<endl;
  return 0;
}
