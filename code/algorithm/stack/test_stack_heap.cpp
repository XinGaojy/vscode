// asan_demo.cpp
#include <iostream>
#include<mcheck.h>
#include <new>
#include <cstring>

void heap_leak() {
    int* p = new int[100];        // 40 B 泄漏
    p[50] = 42;                   // 合法
    // 忘记 delete[] → 泄漏
}

void stack_overflow() {
    char big[20 * 1024 * 1024];   // 20 MB > 默认 8 MB
    big[0] = 1;
}

int main(){
    mtrace();
    heap_leak();
    stack_overflow();             // 会崩
    muntrace();
    return 0;
}
