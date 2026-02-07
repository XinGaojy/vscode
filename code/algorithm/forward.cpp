//实现完美转发
#if 0
#include<iostream>
using namespace std;
void print(int& t){
  cout<<"print(int& t)"<<endl;
}

void print(int&& t){
  cout<<"print(int&& t)"<<endl;
}

template<class T>
void FuncTemplate(T &&t){
  print(t);
  print(std::forward<T>(t));
  print(std::move(t));
}

int main(){
  int i=100;
  FuncTemplate(i);
  FuncTemplate(20);
  return 0;
}
#endif






#include <utility>   // std::forward
#include <iostream>

// 万能工厂：把参数原封不动地转发给 T 的构造函数
template <class T, class... Args>
T create(Args&&... args)          // Args&& 是转发引用（万能引用）
{
    return T(std::forward<Args>(args)...);   // 完美转发核心
}

// 测试类：区分左值/右值构造
struct Foo {
    Foo(int&  ) { std::cout << "左值构造\n"; }
    Foo(int&& ) { std::cout << "右值构造\n"; }
};

int main() {
    int x = 42;

    create<Foo>(x);        // 传入左值 → 左值构造
    create<Foo>(42);       // 传入右值 → 右值构造
    create<Foo>(std::move(x)); // 强制转右值 → 右值构造
    create<Foo>(std::forward<int>(x));
    create<Foo>(std::forward<int>(10));
}
