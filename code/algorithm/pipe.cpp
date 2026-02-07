#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main() {
    int pipefd[2];
    pipe(pipefd);
    
    if (fork() == 0) {
        // 子进程：快速写入（会很快填满管道）
        close(pipefd[0]);
        int count = 0;
        while (1) {
            printf("Writing message %d\n", count++);
            write(pipefd[1], "Hello", 5); // 每次写入5字节
        }
    } else {
        // 父进程：很慢地读取
        close(pipefd[1]);
        sleep(5); // 先等待5秒，让子进程填满管道
        char buf[5];
        while (read(pipefd[0], buf, 5) > 0) {
            printf("Read: Hello\n");
            sleep(1); // 每秒才读一次
        }
    }
    return 0;
}

