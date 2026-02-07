//注意在未声明在编译时报错,未定义在链接时报错
#include <iostream>
using namespace std;
class A {
 public:
  int c;
  static int x;
  void f() {
    c++;
    cout << c << endl;
  }
  static void func() {
    x++;
    cout << x << endl;
    A b;
    cout << b.c << endl;
  }
  void func1() {
    x++;
    cout << x << endl;
  }

  void const_func() const {
    func();
    // f();
    // func1();
  }

  void counter() {
    static int count = 0;
    count++;
    cout << "count"
         << " : " << count << endl;
  }
};



class B{
private:
  int a;
  char b;
  double c;
  static int s;
public:
  static int func(){return 1;}
  virtual int vfunc(){return a;}; 
  virtual int vfunc1()=delete;
};

class C{
  static int a;
  static const int b=1;
  virtual void func(){}
};
int C::a=1;
int A::x = 1;

int main() {
  A a;
  a.c = 100;
  cout << a.x << endl;
  a.func();
  a.func1();
  A::func();
  a.const_func();
  cout << A::x << endl;
  a.counter();
  A b;
  b.counter();
  cout<<sizeof(B)<<endl;
  cout<<sizeof(C)<<endl;
  C c;
  cout<<sizeof(c)<<endl;
  return 0;
}
