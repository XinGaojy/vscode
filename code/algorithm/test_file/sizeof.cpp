#include<iostream>
#include<functional>

using namespace std;

int main(){
    int a=1;
    auto f= [&](){
        cout<<a<<endl;
        cout<<"hello"<<endl;
    };
    function<void()>f1;
    cout<<sizeof(f)<<endl;
    cout<<sizeof(f1)<<endl;
    return 0;
}
