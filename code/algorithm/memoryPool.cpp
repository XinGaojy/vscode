//这里只实现了一个简单版本的内存池，基本思路，开辟一块大的空间，然后通过嵌入式指针的手法将每个块的前 8 个字节作为指针，存放下一块的地址，通过空闲链表串联起来。
#include<iostream>
#include<memory.h>
using namespace std;


class MemoryPool {
public:
  MemoryPool(size_t size, size_t count) : blocksize(size), blockcount(count) {
    memory = (char*)malloc(blocksize * blockcount);
    freelist = memory; // 初始化，freelist指向内存池第一块内容

    char *curr_block = memory;
    for (size_t i = 0; i < count - 1; ++i) {
      // 将每个块的前8个字节作为指针，存放下一个块的地址
      *(void**)curr_block = (void*)(curr_block + blocksize);
      curr_block += blocksize;
    }
    *(void**)curr_block = nullptr;
  }

  ~MemoryPool() {
    free(memory);
  }

  void* allocateBlock() {
    if (freelist == nullptr) {
      return nullptr;
    }
    void *block = freelist;
    freelist = *(void**)block; // freelist指向下一个指针
    return block;
  }

  void deallocate(void* ptr) {
    // 检查内存是否在内存池的范围之内
    if (!(ptr >= memory && ptr < memory + blocksize * blockcount)) {
      return;
    }
    // 将块插入空闲链表的头部
    *(void**)ptr = freelist;
    freelist = ptr;
  }

private:
  char *memory = nullptr;
  void *freelist = nullptr;
  size_t blocksize;
  size_t blockcount;
};

int main() {
  {
    MemoryPool pool(sizeof(int*), 1000);
    int *p = (int*)pool.allocateBlock();

    *p = 10;

    int *p2 = (int*)pool.allocateBlock();
    *p2 = *p;

    std::cout << "&p = " << p << " *p = " << *p << std::endl;
    std::cout << "&p2 = " << p2 << " *p2 = " << *p2 << std::endl;
  }
}
