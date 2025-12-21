#include<stdio.h>
#include <sys/mman.h>
#include <unistd.h>
int main(){
    void *p = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    printf("Parent  virt = %p\n", p);
    if(fork()==0){
        printf("Child   virt = %p\n", p);
        *p = 42;                       // 写共享页
    } else {
        sleep(1);
        printf("Parent  *p   = %d\n", *(int*)p);  // 立即看到 42
    }
    return 0;
}
