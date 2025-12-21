#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main() {
    const char* target = "target.txt";
    const char* hard_link = "hard_link.txt";
    const char* soft_link = "soft_link.txt";

    // 创建目标文件
    FILE* fp = fopen(target, "w");
    fprintf(fp, "Hello, World!\n");
    fclose(fp);

    // 创建硬链接
    if (link(target, hard_link) == 0) {
        printf("硬链接创建成功\n");
    } else {
        perror("创建硬链接失败");
    }

    // 创建软链接
    if (symlink(target, soft_link) == 0) {
        printf("软链接创建成功\n");
    } else {
        perror("创建软链接失败");
    }

    // 检查inode号（相同则为硬链接，不同则为软链接）
    struct stat st1, st2, st3;
    stat(target, &st1);
    stat(hard_link, &st2);
    lstat(soft_link, &st3);  // 用lstat获取软链接本身的信息

    printf("目标文件inode: %lu\n", st1.st_ino);
    printf("硬链接inode: %lu\n", st2.st_ino);
    printf("软链接inode: %lu\n", st3.st_ino);

    // 注意：如果要检查软链接指向的目标的inode，应该用stat而不是lstat
    stat(soft_link, &st3);
    printf("通过软链接获取的目标inode: %lu\n", st3.st_ino);

    return 0;
}
