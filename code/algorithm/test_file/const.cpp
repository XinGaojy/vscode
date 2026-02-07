// 模拟操作系统设置内存保护
#include <iostream>
using namespace std;
int main() {
    int normal = 1;          // 可读写内存页
    const int read_only = 2; // 只读内存页
    
    // 正常访问
    std::cout << normal << std::endl;
    std::cout << read_only << std::endl;
    
    // 尝试绕过 const（危险！）
    int* hack = const_cast<int*>(&read_only);
    *hack = 3;  // 运行时可能触发段错误
    cout<<*hack<<endl;    
    int j=1;
    const int &i_ref=j;
    j=4;
    cout<<j<<endl;
    return 0;
}
