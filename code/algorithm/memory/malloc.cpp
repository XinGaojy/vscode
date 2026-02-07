#include<cstdlib>
#include<iostream>
#include<memory.h>
#include<vector>
#include<new>
using namespace std;
#if 0
int main(){
    int* p=(int*)malloc(10*sizeof(int));
    cout<<p[10]<<endl;
    *(p+100)=10;
    cout<<p[100]<<endl;
    cout<<p[0]<<endl;
//    cout<<p[100000]<<endl;
//    *(p+100000)=10000;
//    cout<<p[100000]<<endl;




    //vector
    cout<<"----------------------"<<endl;
    vector<int>v(10);
    v[20]=10;
    cout<<v[20]<<endl;

    v.at(100000)=11;

//    v[100000]=1;
//    cout<<v[100000]<<endl;

    cout<<"----------------------"<<endl;
    return 0;
}


#endif


#if 0
int main(){
    int *p=new int[10];
    int* ptr=p;
    for(int i=0;i<10;i++){
        *p=i;
        p++;
    }
    p=ptr;
//    delete[] p;
//    free(p);
    delete p;
    cout<<*(ptr+1)<<endl;
    
    cout<<*(ptr+2)<<endl;

    cout<<*(ptr+3)<<endl;
    
    cout<<*(ptr+4)<<endl;
    cout<<*(ptr+5)<<endl;
}


#endif







#if 0
int main(){


#if 0
    char arr[20];              // 栈
    int *p = new  int[20];    // 堆

    cout<<p[10]<<endl;
    cout<<p[30]<<endl;
    cout<<(void*)&arr<<endl;
    cout<<(void*)p<<endl;
    delete[] p;
#endif
struct data{
    int a;
    double b;
};
    
#if 0
    int arr[100];
    data*p=new(arr) data{1,1.1};
    int* ptr=(int*)malloc(1000);
    data*p1=new(ptr) data{2,2.2};
    cout<<p->a<<"---"<<p->b<<endl;
    cout<<p1->a<<"---"<<p1->b<<endl;

#endif


#if 0

    data arr[sizeof(data)*100];
    data*p=new(arr) data{1,1.1};
    data* ptr=(data*)malloc(sizeof(data)*1000);
    data*p1=new(ptr) data{2,2.2};
    cout<<p->a<<"---"<<p->b<<endl;
    cout<<p1->a<<"---"<<p1->b<<endl;
    p->~data();
    p1->~data();
    free(ptr);
//下面的要省略,内置类型没有析构函数
//    p->~int();
//    p1->~int();
    return 0;

#endif

    


}






#endif
int g1 = 42;            // 已初始化 → .data
int g2;                 // 未初始化 → .bss
const int g3 = 100;     // const 全局 → .rodata
static int g4;
static int g5=12;

int main(){
    std::cout << "&g1=" << (void*)&g1 << '\n';
    std::cout << "&g2=" << (void*)&g2 << '\n';
    std::cout << "&g3=" << (void*)&g3 << '\n';
    std::cout << "&g4=" << (void*)&g4 << '\n';
    std::cout << "&g5=" << (void*)&g5 << '\n';
    int a[10];
    a[20]=11;
    std::cout<<a[20]<<std::endl;
    
    cout<<"-------------"<<endl;
    int *p=new int[10];
    p[100]=999;
    cout<<p[100]<<endl;
    delete[] p;
    delete p;
    return 0;
}



