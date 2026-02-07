#if 1

#include<iostream>
#include<memory.h>
#include<functional>
using namespace std;

//实现lambda表达式
class lambda{
private:
    int x;
    int& y;
public:
    lambda(int x1,int&  y1):x(x1),y(y1){}
    inline int operator()(int z)const {return x+y+z;};
};
class lambda_empty{

};

int main(){
#if 0
    int x=1;
    auto lam=[=](int y){return x+y;};
    int res=lam(2);
    cout<<res<<endl;
#endif

#if 1
    int x=1;
    int y=2;
    int z=3;
    int d=4;
    lambda lam(x,y);
    lambda_empty lam_empty;
    int res=lam.operator()(3);
    cout<<res<<endl;
    cout<<sizeof(lam)<<endl;
    cout<<sizeof(lam_empty)<<endl;
    cout<<sizeof(void*)<<endl;
    cout<<sizeof(int)<<endl;
    auto func=[&]()->int{cout<<"hello lambda"<<endl;return 0;}();
    cout<<sizeof(func)<<endl;


    //只要存在指捕获的lambda表达式就不能转换成函数指针,但是可以转换成functionl类

    auto f1 = [x,y]()->int {cout<<"hello lambda1"<<endl;return 0;};
    f1();

    function<void()> f2= [x,y]()->int {cout<<"hello lambda1"<<endl;return 0;};
    f2();
    
    int (*f3)(int,int) = [](int ,int )->int {cout<<"hello lambda1"<<endl;return 0;};
    f3(1,2);
    (*f3)(3,4);

    
    cout<<"--------------------"<<endl;
    function<int(int,int)>ptr = [=](int ,int )->int {cout<<"hello lambda1"<<endl;return 0;};
    ptr(2,3);
    cout<<"--------------------"<<endl;
    

    function<void()>f4 = []()->int {cout<<"hello lambda1"<<endl;return 0;};
    f4();


    //lambda表达式的使用方式
    std::vector<int> numbers = {1, 5, 3, 7, 2, 8, 4, 6};
        
        // 使用 lambda 作为谓词
        std::sort(numbers.begin(), numbers.end(), 
                 [](int a, int b) { return a > b; });  // 降序排序
        
        std::cout << "降序排序: ";
        for (int n : numbers) {
            std::cout << n << " ";
        }
        std::cout << std::endl;

#if 0
        int target=4;
        auto it=remove_if(numbers.begin(),numbers.end(),[target](n){return n<target;});
        numbers.erase(it,numbers.end());
        for(auto i:numbers){
            cout<<i<<endl;
        }

#endif

#endif
        

//计lambda和function的算大小
#if 1
    cout<<"-------------------------------------------------------------"<<endl;
    //小的lambda
        auto small_lambda = []() { return 42; };
    std::cout << "无捕获 lambda 大小: " << sizeof(small_lambda) << " 字节" << std::endl;

    //中等lambda
    int a = 1, b = 2, c = 3;
    auto medium_lambda = [&a, b, c]() { return a + b + c; };
    std::cout << "三个 int 捕获 lambda 大小: " << sizeof(medium_lambda) << " 字节" << std::endl;
    
    //大lambda
    struct BigData {
        char data[100];
    };
    BigData big_data;
    auto big_lambda = [&big_data]() { return sizeof(big_data); };
    std::cout << "大对象捕获 lambda 大小: " << sizeof(big_lambda) << " 字节" << std::endl;




    cout<<"-------------------------------------------------------------"<<endl;
    //funtion内存的使用
    
    // 小函数对象
        std::function<int()> small_func = []() { return 1; };
        std::cout << "小 std::function 大小: " << sizeof(small_func)
                  << " 字节（实际可能因实现而异）" << std::endl;

        // 大函数对象
        struct LargeFunctor {
            char buffer[1000000];
            int operator()() const { return 100; }
        };

        std::function<int()> large_func = LargeFunctor{};
        std::cout << "大 std::function 大小: " << sizeof(large_func) << " 字节" << std::endl;
        std::cout << "调用结果: " << large_func() << std::endl;



#endif



    return 0;
}


#endif



#if 0
#include <iostream>
#include <string>
#include <memory>

void lambda_capture_modes() {
    std::cout << "\n=== Lambda 捕获方式详解 ===" << std::endl;
    
    int a = 1;
    int b = 2;
    int c = 3;
    static int d = 4;  // 静态变量
    
    // 1. 值捕获 [=] 的底层
    std::cout << "\n1. 值捕获 [=]：" << std::endl;
    {
        auto lambda1 = [=]() {
            return a + b + c;  // 捕获所有外部变量（按值）
        };
        
        // 编译器生成的类
        class __lambda_2 {
        private:
            int a;  // 值捕获
            int b;  // 值捕获
            int c;  // 值捕获
            
        public:
            __lambda_2(int a, int b, int c) : a(a), b(b), c(c) {}
            
            int operator()() const {
                return a + b + c;
            }
        };
        
        std::cout << "lambda1() = " << lambda1() << std::endl;
    }
    
    // 2. 引用捕获 [&] 的底层
    std::cout << "\n2. 引用捕获 [&]：" << std::endl;
    {
        auto lambda2 = [&]() {
            a = 10;  // 修改外部变量
            return a + b + c;
        };
        
        // 编译器生成的类
        class __lambda_3 {
        private:
            int& a;  // 引用捕获
            int& b;  // 引用捕获
            int& c;  // 引用捕获
            
        public:
            __lambda_3(int& a, int& b, int& c) : a(a), b(b), c(c) {}
            
            int operator()() const {  // 注意：虽然是 const，但可以修改引用
                a = 10;
                return a + b + c;
            }
        };
        
        std::cout << "修改前 a = " << a << std::endl;
        std::cout << "lambda2() = " << lambda2() << std::endl;
        std::cout << "修改后 a = " << a << std::endl;
    }
    
    // 3. 混合捕获
    std::cout << "\n3. 混合捕获：" << std::endl;
    {
        auto lambda3 = [a, &b, c]() mutable {
            a = 100;  // 可以修改，因为 mutable
            b = 200;  // 修改外部变量
            return a + b + c;
        };
        
        // 编译器生成的类
        class __lambda_4 {
        private:
            int a;  // 值捕获，但可以修改
            int& b; // 引用捕获
            int c;  // 值捕获
            
        public:
            __lambda_4(int a, int& b, int c) : a(a), b(b), c(c) {}
            
            int operator()() {  // 非 const，因为有 mutable
                a = 100;
                b = 200;
                return a + b + c;
            }
        };
        
        std::cout << "lambda3() = " << lambda3() << std::endl;
    }
    
    // 4. 初始化捕获（C++14）
    std::cout << "\n4. 初始化捕获（C++14）：" << std::endl;
    {
        std::unique_ptr<int> ptr = std::make_unique<int>(42);
        
        auto lambda4 = [value = std::move(ptr)]() {
            return *value;
        };
        
        // 编译器生成的类
        class __lambda_5 {
        private:
            std::unique_ptr<int> value;  // 移动捕获
            
        public:
            __lambda_5(std::unique_ptr<int>&& ptr) 
                : value(std::move(ptr)) {}
            
            int operator()() const {
                return *value;
            }
        };
        
        std::cout << "lambda4() = " << lambda4() << std::endl;
    }
#if 0    
    // 5. 泛型 Lambda（C++14）
    std::cout << "\n5. 泛型 Lambda（C++14）：" << std::endl;
    {
        auto lambda5 = [](auto x, auto y) {
            return x + y;
        };
        
        // 编译器生成的类
        class __lambda_6 {
        public:
            template<typename T1, typename T2>
            auto operator()(T1 x, T2 y) const {
                return x + y;
            }
        };
        
        std::cout << "lambda5(1, 2) = " << lambda5(1, 2) << std::endl;
        std::cout << "lambda5(3.14, 2.71) = " << lambda5(3.14, 2.71) << std::endl;
    }
    
#endif

    // 6. 捕获 this
    std::cout << "\n6. 捕获 this：" << std::endl;
    {
        class MyClass {
        private:
            int value = 42;
            
        public:
            void test() {
                // 捕获 this
                auto lambda6 = [this]() {
                    return value;  // 访问成员变量
                };
                
                // 编译器生成的类
                class __lambda_7 {
                private:
                    MyClass* this_ptr;  // 捕获 this
                    
                public:
                    __lambda_7(MyClass* this_ptr) : this_ptr(this_ptr) {}
                    
                    int operator()() const {
                        return this_ptr->value;
                    }
                };
                
                std::cout << "lambda6() = " << lambda6() << std::endl;
            }
        };
        
        MyClass obj;
        obj.test();
    }
    
    // 7. Lambda 的大小
    std::cout << "\n7. Lambda 的大小：" << std::endl;
    {
        // 无捕获的 lambda
        auto lambda7 = []() { return 42; };
        std::cout << "无捕获 lambda 大小: " << sizeof(lambda7) << " 字节" << std::endl;
        
        // 值捕获
        int val = 100;
        auto lambda8 = [val]() { return val; };
        std::cout << "值捕获 lambda 大小: " << sizeof(lambda8) << " 字节" << std::endl;
        
        // 引用捕获
        auto lambda9 = [&val]() { return val; };
        std::cout << "引用捕获 lambda 大小: " << sizeof(lambda9) << " 字节" << std::endl;
        
        // 多个捕获
        double dbl = 3.14;
        std::string str = "hello";
        auto lambda10 = [val, dbl, &str]() { 
            return val + dbl + str.length(); 
        };
        std::cout << "多个捕获 lambda 大小: " << sizeof(lambda10) << " 字节" << std::endl;
    }
}




int main(){

    lambda_capture_modes();
    return 0;
}

#endif


