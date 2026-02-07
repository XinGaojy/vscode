#include<iostream>
using namespace std;
class A{
public:
    void func(){
        cout<<"hello"<<endl;
        func1();
    }
    virtual void vir_func(){
        func();
    }
    void func1(){
        cout<<"hello1"<<endl;
    }
};
int main(){
    A a;
    a.vir_func();
    return 0;
}
