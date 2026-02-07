
#if 1
#include<iostream>
#include<memory.h>
using namespace std;
class Simple {
private:
    static int x;
    int y;

public:
    void print() { cout << x << ", " << y << endl; }
};
int Simple::x=10;
int main(){
  Simple s;
  memset(&s, 0, sizeof(s));  // ✅ 安全
  cout<<Simple::x<<endl;
  s.print();                 // 正常输出: 0, 0
  return 0;
}


#endif

#if 0
#include<iostream>
#include<memory.h>
using namespace std;
class Base {
public:
    virtual void func() { cout << "Base::func" << endl; }

private:
    int data;
};

int main(){
  Base b;
  memset(&b, 0, sizeof(b));  // ❌ 灾难！
  b.func();                  // 程序崩溃！
  return 0;
}
#endif

#if 0

#include<iostream>
#include<string>
#include<memory.h>
using namespace std;

class MyClass {
public:
    int data = 100;
    std::string str = "Hello";
    
    void print() {
        std::cout << "data: " << data << ", str: " << str << std::endl;
    }
    
    void setData(int val) {
        data = val;
    }
    
    virtual void virtualFunc() {
        std::cout << "Virtual function called" << std::endl;
    }
};

int main() {
    MyClass obj;
    
    std::cout << "调用memset前:" << std::endl;
    obj.print();  // 正常输出: data: 100, str: Hello
    
    // 危险操作!
    memset(&obj, 0, sizeof(obj));
    
    std::cout << "调用memset后:" << std::endl;
    // 这里会发生什么?
    obj.print();  // 未定义行为!
    
    return 0;
}



#endif



#if 0
#include<iostream>
#include<memory.h>
#include<string>
using namespace std;

class Base {
public:
    int data = 10;
    
    Base() {
        std::cout << "Base constructor" << std::endl;
    }
    
    virtual ~Base() {
        std::cout << "Base destructor" << std::endl;
    }
    
    virtual void show() {
        std::cout << "Base::show(), data = " << data << std::endl;
    }
    
    void normalFunc() {
        std::cout << "Base::normalFunc()" << std::endl;
    }
};

int main() {
    Base* obj = new Base();
    
    std::cout << "\n调用memset前:" << std::endl;
    obj->show();         // 正常调用虚函数
    obj->normalFunc();   // 正常调用普通函数
    
    std::cout << "\n虚函数表地址: " << *(void**)obj << std::endl;
    
    // 危险的memset!
    memset(obj, 0, sizeof(*obj));
    
    std::cout << "\n调用memset后:" << std::endl;
    std::cout << "虚函数表地址: " << *(void**)obj << std::endl;  // 输出: 0x0!
    
    // 下面这行会导致段错误！
    // obj->show();  // 试图通过空指针调用虚函数
    
    // 普通函数可以调用吗？
    obj->normalFunc();  // 理论上可以，但实际上也很危险
    
    // 尝试delete也会崩溃！
    // delete obj;  // 段错误，虚表被破坏
    
    // 必须手动调用析构函数（但这是错误的）
    obj->~Base();  // 可能崩溃，因为虚表指针为空
    
    delete obj;  // 内存泄漏+潜在崩溃
    
    return 0;
}
#endif


#if 0
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
using namespace std;
class ContainerClass {
public:
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::string str = "Test String";
    
    void display() {
        std::cout << "Vector: ";
        for (int n : vec) std::cout << n << " ";
        std::cout << "\nString: " << str << std::endl;
    }
    
    ~ContainerClass() {
        std::cout << "Destructor called" << std::endl;
    }
};

int main() {
    ContainerClass obj;
    
    std::cout << "Before memset:" << std::endl;
    obj.display();  // 正常输出
    
    // 危险操作！
    memset(&obj, 0, sizeof(obj));
    
    std::cout << "\nAfter memset:" << std::endl;
    // 下面这行会导致段错误！
     obj.display();  // vector和string的内部指针被清零了！
    
    // 程序退出时会发生双重释放！
    // vector和string会在析构时尝试释放已经无效的内存
    return 0;  // 这里会崩溃！
}

#endif




