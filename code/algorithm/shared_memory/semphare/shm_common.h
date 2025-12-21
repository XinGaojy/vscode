// shm_common.h
#ifndef SHM_COMMON_H
#define SHM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

// 生成一个唯一的键值，用于标识共享内存和信号量
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

// 共享内存中存储的数据结构
struct shared_data {
    int count; // 计数器
    char text[256]; // 一些文本
};

// 联合体，用于semctl操作
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// P操作 - 申请资源
void sem_wait(int sem_id) {
    struct sembuf sb = {0, -1, 0}; // 对第0个信号量进行-1操作
    semop(sem_id, &sb, 1);
}

// V操作 - 释放资源
void sem_signal(int sem_id) {
    struct sembuf sb = {0, 1, 0}; // 对第0个信号量进行+1操作
    semop(sem_id, &sb, 1);
}

#endif
