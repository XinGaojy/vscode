// reader.c
#include "shm_common.h"

int main() {
    int shm_id, sem_id;
    struct shared_data *shm_ptr;

    // 1. 获取已存在的共享内存
    shm_id = shmget(SHM_KEY, sizeof(struct shared_data), 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // 2. 获取已存在的信号量
    sem_id = semget(SEM_KEY, 1, 0666);
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }

    // 3. 连接共享内存
    shm_ptr = (struct shared_data *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    printf("Reader: 开始读取数据（Ctrl+C 退出）\n");

    // 4. 读取循环
    int last_count = -1;
    while (1) {
        // ==== 进入临界区 ====
        sem_wait(sem_id); // P操作，获取锁

        // 安全地读取共享内存
        if (shm_ptr->count != last_count) {
            printf("Reader: 读到数据 - Count: %d, Text: '%s'\n", 
                   shm_ptr->count, shm_ptr->text);
            last_count = shm_ptr->count;
        }

        // ==== 离开临界区 ====
        sem_signal(sem_id); // V操作，释放锁

        usleep(500000); // 休眠0.5秒，避免过于频繁的读取
    }

    // 5. 断开连接（通常不会执行到这里）
    shmdt(shm_ptr);
    return 0;
}
