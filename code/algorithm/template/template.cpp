

#if 0
#include<iostream>
#include<memory>
using namespace std;
template<typename T>
T twice(T x) { return x + x; }

int main() {
    twice<int>(42);      // 生成 int 专版
    twice<double>(3.14); // 生成 double 专版
    return 0;
}

#endif



#if 1


//使用模板元编程
#include <iostream>
#include <type_traits>
#include <string>
#include <vector>

/*
元编程（Metaprogramming）是指在编译时生成或操作程序的程序。
在C++中，元编程主要通过模板、constexpr、模板特化等技术实现。

关键特征：
1. 编译时计算
2. 类型计算
3. 代码生成
4. 零开销抽象
*/

// 基本示例：编译时计算斐波那契数列
template<int N>
struct Fibonacci {
    static constexpr int value = Fibonacci<N-1>::value + Fibonacci<N-2>::value;
};

template<>
struct Fibonacci<0> {
    static constexpr int value = 0;
};

template<>
struct Fibonacci<1> {
    static constexpr int value = 1;
};

void demonstrate_basics() {
    std::cout << "=== 元编程基础示例 ===\n";
    std::cout << "斐波那契数列（编译时计算）：\n";
    std::cout << "F(0) = " << Fibonacci<0>::value << "\n";
    std::cout << "F(1) = " << Fibonacci<1>::value << "\n";
    std::cout << "F(5) = " << Fibonacci<5>::value << "\n";
    std::cout << "F(10) = " << Fibonacci<10>::value << "\n";
    std::cout << "F(20) = " << Fibonacci<20>::value << "\n";
    
    // 这些值在编译时就已经计算完成
    constexpr int fib10 = Fibonacci<10>::value;  // 编译时常量
    int array[Fibonacci<5>::value];  // 数组大小在编译时确定
    std::cout << "使用F(5)作为数组大小: " << sizeof(array)/sizeof(array[0]) << "\n";
}
int main(){
    demonstrate_basics();
    return 0;
}




#endif



