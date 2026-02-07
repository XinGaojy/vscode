#include "file.h"

int gCount = 1;             // 全局定义（外部链接）
static int sCount = 2;      // 内部链接，仅本文件可见

void foo() {
    static int calls = 0;   // 函数级静态
    ++calls;
    std::cout << calls << '\n';
}
