#include <iostream>
#include<vector>
#include<memory.h>
#if 0
int main() {
      
//    int a[40000000];   // 40 MB,在栈上分配,glibc默认是8M,直接溢出

   // int a[40000000];

    //static int a[2000000000];是在.bss/.data上分配内存,需要通过mmap将内存映射,所以需要代码
    
    //std::vector<int>vec(1000000);//底层给是调用new,所以超过内存限制会报bad_alloc


    //int *p=(int*)malloc(sizeof(int)*100000000000000000);//可以在虚拟内存上分配大于实际物理内存的空间,只要开启swap区,但是对其进行写操作会直接报错
                                            //
//    std::cout << vec[0] << '\n';
    //*p=10;
    //std::cout<<*p<<std::endl;
    //free(p);
    
//    int *p=new int[100000*sizeof(int)];
//    free(p);
//     *p=10;
    return 0;
}


#endif

#if 0
int main() {
    try{

        //    int a[40000000];   // 40 MB,在栈上分配,glibc默认是8M,直接溢出

            //int a[2000000000];

            //static int a[2000000000];是在.bss/.data上分配内存,需要通过mmap将内存映射,所以需要代码
            
            std::vector<int>vec(1000000);//底层给是调用new,所以超过内存限制会报bad_alloc


            int *p=(int*)malloc(100000000000000000);//可以在虚拟内存上分配大于实际物理内存的空间,只要开启swap区,但是对其进行写操作会直接报错
                                                    //
        //    std::cout << vec[0] << '\n';
            *p=10;
            std::cout<<*p<<std::endl;
            free(p);
        }catch(const std::bad_alloc& e){
            std::cout<<"error"<<std::endl;
        }
    return 0;
}

#endif



#if 1

#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    if (sig == SIGSEGV) {
        std::cout << "[SIGSEGV] 栈溢出！\n";
        _exit(1);
    }
}

// 每帧消耗 ≈ 8 B (int + 指针)
void recurse(int depth) {
    volatile int a[2] = {depth, depth};   // 8 B / 帧
    if (depth > 0) recurse(depth - 1);
}

int main(int argc, char* argv[]){
    signal(SIGSEGV, handler);
    int maxDepth = argc > 1 ? std::atoi(argv[1]) : 1000000;
    std::cout << "准备递归 " << maxDepth << " 层...\n";
    recurse(maxDepth);
    std::cout << "正常返回\n";
    return 0;
}

#endif


