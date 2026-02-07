
#if 0
#include <iostream>
using namespace std;
struct Base {
    Base(){cout<<"Base"<<endl;}
     ~Base() { std::cout << "Base::~Base\n"; }   // ✅ 虚析构
};

struct Derived : public Base {

    Derived(){cout<<"Derived"<<endl;}
    ~Derived()  { std::cout << "Derived::~Derived\n"; }
};

int main() {
    Base* b = new Derived;
    delete b;   // ✅ 完整析构：Derived + Base
}

#endif


#if 1
#include <iostream>
using namespace std;

#if 0
struct A { int x; };                    // 非虚
struct B : A { int y; };                // 非虚
struct C : A { virtual void foo(); };   // 有虚函数 → +vptr
int main(){
    std::cout << "A: " << sizeof(A) << '\n';   // 4
    std::cout << "B: " << sizeof(B) << '\n';   // 8
    std::cout << "C: " << sizeof(C) << '\n';   // 16（+8 vptr，64-bit 对齐）


}

#endif

#if 0
struct A { int x; };                            // 非虚基类
struct B : virtual A { int y; };                // 虚继承 → +vbase 指针
struct C : virtual A { int z; };                // 虚继承 → +vbase 指针
struct D : B, C { int w; };                     // 钻石继承


int main(){
    std::cout << "A: " << sizeof(A) << '\n';        // 4
    std::cout << "B: " << sizeof(B) << '\n';        // 16（+8 vbase 指针）
    std::cout << "C: " << sizeof(C) << '\n';        // 16（+8 vbase 指针）
    std::cout << "D: " << sizeof(D) << '\n';        // 24（+8 vbase B + 8 vbase C + 对齐）


}

#endif


#if 0
struct A { virtual void foo(); };               // 有虚函数 → +vptr
struct B : virtual A { virtual void bar(); };   // 虚继承 + 有虚函数 → +vptr + vbase
struct C : virtual A { virtual void baz(); };   // 虚继承 + 有虚函数 → +vptr + vbase
struct D : B, C { int w; };                     // 钻石继承


int main(){
    std::cout << "A: " << sizeof(A) << '\n';        
    std::cout << "B: " << sizeof(B) << '\n';       
    std::cout << "C: " << sizeof(C) << '\n';        
    std::cout << "D: " << sizeof(D) << '\n'; 


}

#endif


#if 0
class Base {
    int data;  // 4字节
public:
    Base(int d) : data(d) {}
    virtual ~Base() {}  // 虚析构函数
};                      // 大小: 16字节

class Derived1 : virtual public Base {  // 虚继承
    int derived1_data;  // 4字节
public:
    Derived1() : Base(0), derived1_data(0) {}
};                      // 大小: 增加虚基类指针

class Derived2 : virtual public Base {  // 虚继承
    int derived2_data;  // 4字节
public:
    Derived2() : Base(0), derived2_data(0) {}
};                      // 大小: 增加虚基类指针

class MostDerived : public Derived1, public Derived2 {
    int most_derived_data;  // 4字节
public:
    MostDerived() : Base(0), Derived1(), Derived2(), 
                    most_derived_data(0) {}
};  // 大小: 更复杂的内存布局

int main() {
    cout << "Base: " << sizeof(Base) << endl;                // 16
    cout << "Derived1 (虚继承): " << sizeof(Derived1) << endl;  // 32
    cout << "Derived2 (虚继承): " << sizeof(Derived2) << endl;  // 32
    cout << "MostDerived: " << sizeof(MostDerived) << endl;   // 56
    
    MostDerived md;
    // 内存布局（典型实现，不同编译器可能不同）：
    /*
    MostDerived对象：
    +----------------+  <- 对象起始地址
    | Derived1 vptr  | 指向Derived1虚表
    +----------------+
    | derived1_data  | Derived1的数据
    +----------------+
    | Derived2 vptr  | 指向Derived2虚表
    +----------------+
    | derived2_data  | Derived2的数据
    +----------------+
    | vptr to Base   | 虚基类指针（指向Base子对象）
    +----------------+
    | most_derived_data | MostDerived的数据
    +----------------+
    | 填充           | 对齐
    +----------------+
    | Base vptr      | Base的虚表指针
    +----------------+
    | Base::data     | Base的数据（只有一份！）
    +----------------+
    | 填充           | 对齐
    +----------------+
    总大小: 8+4+8+4+8+4+4+8+4+4 = 56字节
    */
    
    return 0;
}

#endif






#endif


#if 0
#include <iostream>

class Base {
public:
    virtual void foo(int x = 1) { std::cout << "Base::foo(" << x << ")\n"; }
};

class Derived : public Base {
public:
    void foo(int x = 2) override { std::cout << "Derived::foo(" << x << ")\n"; }
};

int main() {
    Base* b = new Derived;
    Derived* c = new Derived;
    b->foo();           // 动态分派：Derived::foo
    c->foo();
    // 默认参数由静态类型 Base 决定，x = 1
    return 0;
}


#endif

#if 0
#include <iostream>

class Base {
public:
    virtual void foo(int x) { std::cout << "Base::foo(int)\n"; }
    virtual void foo(double x) { std::cout << "Base::foo(double)\n"; }
};

class Derived : public Base {
public:
    void foo(int x) override { std::cout << "Derived::foo(int)\n"; }
    // 没有重写 Base::foo(double)
};

int main() {
    Base* b = new Derived;
    b->foo(3.14);           // 候选集 = {Base::foo(int), Base::foo(double)}
                            // 最佳匹配 = Base::foo(double)
                            // 动态分派 = Base::foo(double)（因为 Derived 没有重写它）
    b->foo(1);
    return 0;
}

#endif



#if 0

#include <iostream>
#include<cstdlib>
void by_ptr(int* p) {
    if (p) std::cout << *p << '\n';   // 可能空
}

void by_ref(int& r) {
    std::cout << r << '\n';          // 不可空
}

void by_ref_ref(int&& rr){
    std::cout<<rr<<'\n';
    std::cout<<"&&"<<std::endl;
    std::cout<<__func__<<std::endl;
}


#define LOG(msg) std::cout << "[" << __FILE__ << ":" << __LINE__ << "] " << msg << '\n'

int main() {
    int x = 42;
    by_ref(x);                       // 不可空
    by_ref_ref(20);
    std::cout<<__FILE__<<std::endl;
    std::cout<<__LINE__<<std::endl;
    
    cout<<sizeof(void)<<endl;    

    cout<<sizeof(void)<<endl;    
    cout<<__func__<<endl;
        
    LOG("Hello");

}


#endif


#if 0

#include <iostream>

class Foo { };   // 空壳

int main() {
    std::cout << "默认构造: " << sizeof(Foo) << '\n';           // 1（空类）
    std::cout << "默认对齐: " << alignof(Foo) << '\n';         // 1（实现定义）
    std::cout << "默认构造: " << sizeof(Foo()) << '\n';        // 1（默认构造）
    return 0;

}


#endif


#include <iostream>

void myFunc() {
    std::cout << "在 main 之前执行\n";
}

class Init {
public:
    Init() { myFunc(); }
};

static Init init;  // 全局对象，构造函数在 main 之前执行

int main() {
    std::cout << "main 函数\n";
    return 0;
}
