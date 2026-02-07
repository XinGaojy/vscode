// 示例源码：example.c
#include <stdio.h>
#define PI 3.14159
#define MAX(a,b) ((a)>(b)?(a):(b))

int main() {
    printf("PI = %f\n", PI);
    int x = MAX(5, 3);
    return 0;
}
