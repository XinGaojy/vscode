// writer.c
#include "shm_common.h"

int main() {
    int shm_id, sem_id;
    struct shared_data *shm_ptr;
    union semun sem_arg;

    // 1. 创建共享内存
    shm_id = shmget(SHM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // 2. 创建信号量集（包含1个信号量）
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget failed");
        // 清理共享内存
        shmctl(shm_id, IPC_RMID, NULL);
        exit(1);
    }

    // 3. 初始化信号量的值为1（二进制信号量，互斥锁）
    sem_arg.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_arg) == -1) {
        perror("semctl SETVAL failed");
        // 清理资源
        semctl(sem_id, 0, IPC_RMID);
        shmctl(shm_id, IPC_RMID, NULL);
        exit(1);
    }

    // 4. 连接共享内存
    shm_ptr = (struct shared_data *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    printf("Writer: 开始写入数据（输入 'quit' 退出）\n");

    // 5. 写入循环
    while (1) {
        char input[256];

        printf("请输入文本: ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // 去除换行符
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0) break;

        // ==== 进入临界区 ====
        sem_wait(sem_id); // P操作，获取锁

        // 安全地写入共享内存
        shm_ptr->count++;
        strncpy(shm_ptr->text, input, sizeof(shm_ptr->text) - 1);
        shm_ptr->text[sizeof(shm_ptr->text) - 1] = '\0'; // 确保字符串结尾

        printf("Writer: 写入成功 - Count: %d, Text: '%s'\n", shm_ptr->count, shm_ptr->text);
        // ==== 离开临界区 ====
        sem_signal(sem_id); // V操作，释放锁

        sleep(1); // 模拟一些处理时间
    }

    // 6. 清理资源
    shmdt(shm_ptr);
    printf("Writer: 退出并清理资源\n");
    semctl(sem_id, 0, IPC_RMID); // 删除信号量
    shmctl(shm_id, IPC_RMID, NULL); // 删除共享内存

    return 0;
}
