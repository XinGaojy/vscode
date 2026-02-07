#if 0
//使用将无符号整数转换成字符串的方式



#include <iostream>
#include<string>
using namespace std;
// 最简单的字节序检测
bool is_little_endian() {
    int num = 1;
    return *(char*)&num == 1;  // 检查第一个字节是否为1
}


int main() {
    std::cout << (is_little_endian() ? "小端序" : "大端序") << std::endl;
    return 0;
}

#endif


#if 0
#include <stdio.h>
int main() {
    unsigned int x = 0x12345678;
    char *c = (char*)&x;
    if (*c == 0x78) {
        printf("系统是小端序\n");
    } else {
        printf("系统是大端序\n");
    }
    return 0;
}

#endif





#if 0
#include<iostream>
#include<stdio.h>
using namespace std;

int main(){
    unsigned int x=0x12345678;
    char *c=(char*)&x;
    //cout<< *c == 0x78 ? "小端" : "大端";
    if(*c==0x78){
        cout<<"smallduan"<<endl;
    }else{
        cout<<"bigduan"<<endl;
    }
    return 0;
}

#endif

#if 1

//使用union的方式来判断
#include <stdio.h>
union {
    unsigned int u=0x12345678;
    unsigned char c[4];
} testend;
int main() {
    testend.u = 0x12345678;
    if (testend.c[0] == 0x12) {
        printf("大端序（Big-Endian）\n");
    } else if (testend.c[0] == 0x78) {
        printf("小端序（Little-Endian）\n");
    } else {
        printf("无法确定字节序\n");
    }
    return 0;
}


#endif



