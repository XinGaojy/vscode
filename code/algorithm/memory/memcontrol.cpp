#include <iostream>
#include <memory>
#include <vector>

struct Loud {
    Loud()  { std::cout << "Loud@" << this << " 构造\n"; }
    ~Loud() { std::cout << "Loud@" << this << " 析构\n"; }
};

void demo() {
    std::cout << "\n----- 自动存储期 -----\n";
    Loud autoObj;                 // 构造

    std::cout << "\n----- 动态存储期-裸指针 -----\n";
    Loud* raw = new Loud;         // 构造
    delete raw;                   // 析构；忘记=泄漏

    std::cout << "\n----- 动态存储期-智能指针 -----\n";
    //std::unique_ptr<Loud> ptr = std::make_unique<Loud>(); // 构造
    
//    std::unique_ptr<Loud>ptr1=std::unique_ptr<Loud>(new Loud());
    std::shared_ptr<Loud> ptr1=std::shared_ptr<Loud>(new Loud());
    //std::shared_ptr<Loud>ptr2=ptr1;

}                                 // ptr 离开作用域→自动 delete→析构
// autoObj 离开作用域→自动析构

thread_local Loud tl;             // 线程存储期：每线程一份，线程结束析构
static Loud global;                 // 静态存储期：程序结束析构

int main() {
    std::cout << "===== 程序开始 =====\n";
    Loud global;
    demo();
    std::cout << "===== 程序即将结束 =====\n";
    return 0;
}
