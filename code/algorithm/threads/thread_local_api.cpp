#if 0
#include<iostream>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
using namespace std;
pthread_key_t tls_key;

void *f(void *args) {
    (void)args;                       // 消除 unused 参数警告
    int *num = (int *)calloc(1, sizeof(int)); // 强制转换 void* → int*
    pthread_setspecific(tls_key, num);
    int *val = (int *)pthread_getspecific(tls_key); // 强制转换
    cout<<"val"<<*val<<endl;
    assert(*val == 0);
    return NULL;                      // 消除无返回警告
}

int main() {
    pthread_t t1;
    pthread_attr_t attr;
    int err = pthread_key_create(&tls_key, NULL);
    assert(err == 0);
    err = pthread_attr_init(&attr);
    assert(err == 0);
    err = pthread_create(&t1, &attr, f, NULL);
    assert(err == 0);
    pthread_join(t1, NULL);
    int *val = (int *)pthread_getspecific(tls_key); // 强制转换
    // 每个线程的tls_key是独享的，主线程没有设置tls_key对应的内存区域，因此val的值为null
    assert(val == NULL);
    return 0;
}



#endif





#if 0
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include<memory.h>

pthread_key_t tls_key;

void *f(void *args) {
  int *num = calloc(1, sizeof(int));
  pthread_setspecific(tls_key, num);
  int *val = pthread_getspecific(tls_key);
  assert(*val == 0);
}

int main() {
  pthread_t t1;
  pthread_attr_t attr;
  int err = pthread_key_create(&tls_key, NULL);
  assert(err == 0);
  err = pthread_attr_init(&attr);
  assert(err == 0);
  err = pthread_create(&t1, &attr, f, NULL);
  assert(err == 0);
  pthread_join(t1, NULL);
  int *val = pthread_getspecific(tls_key);
  // 每个线程的tls_key是独享的，主线程没有设置tls_key对应的内存区域，因此val的值为null
  assert(val == NULL);
  return 0;
}

#endif



#if 1
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

/* 全局 TLS key */
static pthread_key_t tls_key;

/* 自动释放内存的析构函数 */
static void tls_destructor(void *ptr)
{
    printf("tls_destructor: free %p (thread %lu)\n",
           ptr, (unsigned long)pthread_self());
    free(ptr);
}

/* 线程工作函数 */
static void *thread_func(void *arg)
{
    /* 分配线程私有数据 */
    int *num = (int*)calloc(1, sizeof(int));
    assert(num != NULL);

    /* 设置 TLS */
    int err = pthread_setspecific(tls_key, num);
    assert(err == 0);

    /* 使用 TLS */
    int *val = (int*)pthread_getspecific(tls_key);
    assert(val != NULL);
    assert(*val == 0);          // calloc 初始为 0

    *val = 42;                  // 每个线程写自己的副本
    printf("thread %lu: val=%d\n", (unsigned long)pthread_self(), *val);

    return NULL;
}

int main(void)
{
    int err;

    /* 1. 创建 TLS key，指定析构函数 */
    err = pthread_key_create(&tls_key, tls_destructor);
    assert(err == 0);

    /* 2. 主线程也设置自己的私有数据 */
    int *main_num = (int*)calloc(1, sizeof(int));
    assert(main_num != NULL);
    *main_num = 99;
    err = pthread_setspecific(tls_key, main_num);
    assert(err == 0);

    /* 3. 创建工作线程 */
    pthread_t t1;
    err = pthread_create(&t1, NULL, thread_func, NULL);
    assert(err == 0);
    pthread_join(t1, NULL);

    /* 4. 验证主线程副本未被子线程破坏 */
    int *main_val = (int*)pthread_getspecific(tls_key);
    assert(main_val == main_num);
    assert(*main_val == 99);
    printf("main thread: val=%d\n", *main_val);

    /* 5. 删除 key → 会触发主线程的析构函数 */
    err = pthread_key_delete(tls_key);
    assert(err == 0);

    return 0;
}


#endif


