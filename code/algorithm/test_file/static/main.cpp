#include "file.h"

int main() {
    std::cout << gCount << '\n';  // 0
    foo();                        // 1
    // sCount 不可访问——内部链接
    return 0;
}
