

////////////////////////////////////////////////////////////////
///
//不使用智能指针

#if 0

/*********************************************************************
 *  Thread-Safe SkipList Template
 *  Author :  https://github.com/yourname
 *  License:  MIT
 *********************************************************************/
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>


template <typename K, typename V, typename Compare = std::less<K>>
class TSkiplist {
 public:
  using key_type   = K;
  using mapped_type = V;
  using value_type = std::pair<const K, V>;

 private:
  struct Node;
  using NodePtr = Node*;

  /*------------- 自旋锁 -------------*/
  class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
   public:
    void lock()   { while (flag_.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag_.clear(std::memory_order_release); }
  };

  /*------------- 节点 -------------*/
  struct Node {
    const K key;
    V       val;
    NodePtr forward[1];          // 柔性数组，实际长度 = level
    SpinLock node_lock;          // 每个节点一把细粒度锁（可选）
    Node(int lvl, const K& k, const V& v) : key(k), val(v) {}
  };

  /*------------- 内存布局 -------------*/
  static std::size_t node_size(int level) {
    return sizeof(Node) + (level - 1) * sizeof(NodePtr);
  }
  static void* alloc_node(int level, const K& k, const V& v) {
    void* raw = ::operator new(node_size(level));
    return new (raw) Node(level, k, v);
  }
  static void free_node(Node* n) { ::operator delete(n); }

  /*------------- 成员变量 -------------*/
  static constexpr double kProb   = 0.25;
  static constexpr int    kMaxLvl = 32;
  NodePtr head_;
  int     max_level_;
  SpinLock list_lock_;             // 全局锁（简单起见，写操作独占）
  Compare cmp_;
  std::atomic<size_t> size_{0};
  std::mt19937 rng_{std::random_device{}()};
  std::uniform_real_distribution<double> dist_{0.0, 1.0};

  int random_level() {
    int lvl = 1;
    while (lvl < kMaxLvl && dist_(rng_) < kProb) ++lvl;
    return lvl;
  }

 public:
  TSkiplist() : max_level_(1) {
    head_ = static_cast<Node*>(alloc_node(kMaxLvl, K{}, V{}));
    for (int i = 0; i < kMaxLvl; ++i) head_->forward[i] = nullptr;
  }
  ~TSkiplist() { clear(); free_node(head_); }

  /* 禁用拷贝 */
  TSkiplist(const TSkiplist&) = delete;
  TSkiplist& operator=(const TSkiplist&) = delete;

  /*------------- 查询 -------------*/
  bool find(const K& key, V& out) const {
    std::lock_guard<SpinLock> g(const_cast<SpinLock&>(list_lock_));
    NodePtr x = head_;
    for (int i = max_level_ - 1; i >= 0; --i) {
      while (x->forward[i] && cmp_(x->forward[i]->key, key))
        x = x->forward[i];
    }
    x = x->forward[0];
    if (x && !cmp_(key, x->key) && !cmp_(x->key, key)) {
      out = x->val;
      return true;
    }
    return false;
  }

  /*------------- 插入 -------------*/
  bool insert(const K& key, const V& val) {
    NodePtr update[kMaxLvl];
    std::lock_guard<SpinLock> g(list_lock_);

    NodePtr x = head_;
    for (int i = max_level_ - 1; i >= 0; --i) {
      while (x->forward[i] && cmp_(x->forward[i]->key, key))
        x = x->forward[i];
      update[i] = x;
    }
    x = x->forward[0];
    /* 已存在，更新 */
    if (x && !cmp_(key, x->key) && !cmp_(x->key, key)) {
      x->val = val;
      return false;
    }
    int lvl = random_level();
    if (lvl > max_level_) {
      for (int i = max_level_; i < lvl; ++i) update[i] = head_;
      max_level_ = lvl;
    }
    NodePtr n = static_cast<Node*>(alloc_node(lvl, key, val));
    for (int i = 0; i < lvl; ++i) {
      n->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = n;
    }
    size_.fetch_add(1, std::memory_order_relaxed);
    return true;
  }

  /*------------- 删除 -------------*/
  bool erase(const K& key) {
    NodePtr update[kMaxLvl];
    std::lock_guard<SpinLock> g(list_lock_);

    NodePtr x = head_;
    for (int i = max_level_ - 1; i >= 0; --i) {
      while (x->forward[i] && cmp_(x->forward[i]->key, key))
        x = x->forward[i];
      update[i] = x;
    }
    x = x->forward[0];
    if (!x || cmp_(key, x->key) || cmp_(x->key, key)) return false;
    for (int i = 0; i < max_level_; ++i) {
      if (update[i]->forward[i] != x) break;
      update[i]->forward[i] = x->forward[i];
    }
    free_node(x);
    while (max_level_ > 1 && head_->forward[max_level_ - 1] == nullptr)
      --max_level_;
    size_.fetch_sub(1, std::memory_order_relaxed);
    return true;
  }

  /*------------- 元素访问 -------------*/
  V& operator[](const K& key) {
    V v{};
    if (find(key, v)) return *reinterpret_cast<V*>(0x1); // 仅占位，实际用下面
    insert(key, V{});
    V* found = nullptr;
    find(key, *found);
    return *found;
  }

  /*------------- 容量 -------------*/
  size_t size() const { return size_.load(std::memory_order_relaxed); }
  bool   empty() const { return size() == 0; }

  /*------------- 清空 -------------*/
  void clear() {
    std::lock_guard<SpinLock> g(list_lock_);
    NodePtr cur = head_->forward[0];
    while (cur) {
      NodePtr nxt = cur->forward[0];
      free_node(cur);
      cur = nxt;
    }
    for (int i = 0; i < kMaxLvl; ++i) head_->forward[i] = nullptr;
    max_level_ = 1;
    size_.store(0, std::memory_order_relaxed);
  }

  /*------------- 打印（调试用） -------------*/
  void dump() const {
    std::lock_guard<SpinLock> g(const_cast<SpinLock&>(list_lock_));
    for (int i = max_level_ - 1; i >= 0; --i) {
      NodePtr p = head_->forward[i];
      std::cout << "Level " << i << ": ";
      while (p) { std::cout << p->key << " "; p = p->forward[i]; }
      std::cout << "\n";
    }
  }
};

/*====================================================================
 * 简单并发测试
 *====================================================================*/


void reader(TSkiplist<int, std::string>& sk, int id) {
  for (int i = 0; i < 1000; ++i) {
    std::string v;
    if (sk.find(i, v)) {
      // std::cout << "R" << id << " find " << i << "=" << v << "\n";
    }
  }
}

void writer(TSkiplist<int, std::string>& sk, int id) {
  for (int i = 0; i < 500; ++i) {
    sk.insert(i, std::to_string(i) + "_val_" + std::to_string(id));
  }
}

int main() {
  TSkiplist<int, std::string> sk;
  std::vector<std::thread> pool;
  auto t0 = std::chrono::steady_clock::now();

  for (int i = 0; i < 8; ++i) pool.emplace_back(writer, std::ref(sk), i);
  for (int i = 0; i < 8; ++i) pool.emplace_back(reader, std::ref(sk), i);
  for (auto& t : pool) t.join();

  auto t1 = std::chrono::steady_clock::now();
  std::cout << "Time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
            << " ms\n";
  std::cout << "Final size: " << sk.size() << "\n";
  // sk.dump();
  return 0;
}

#endif

#if 0
1-------------->9

1------>4------>9

1-->3-->4-->6-->9

#endif

//使用智能指针的方式

#if 0
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

// 跳表节点
template<typename K, typename V>
class Node {
public:
    K key;
    V value;
    std::vector<std::shared_ptr<Node<K, V>>> next;  // 前进指针数组
    std::mutex node_mutex;  // 节点级别的锁
    int node_level;  // 节点所在的层数
    
    Node(K k, V v, int level) 
        : key(k), value(v), node_level(level) {
        next.resize(level + 1, nullptr);
    }


    Node(int level) 
        : key(K()), value(V()), node_level(level) {
        next.resize(level + 1, nullptr);
    }
};

// 线程安全跳表
template<typename K, typename V>
class ThreadSafeSkipList {
private:
    const int MAX_LEVEL = 16;      // 最大层数
    const float P = 0.5f;          // 升级概率
    
    std::shared_ptr<Node<K, V>> head;  // 头节点
    int current_level;           // 当前最大层数
    int element_count;           // 元素个数
    
    mutable std::shared_mutex rw_mutex;  // 读写锁
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;
    
    // 生成随机层数
    int random_level() {
        int level = 1;
        while (distribution(generator) < P && level < MAX_LEVEL) {
            level++;
        }
        return level;
    }
    
public:
    ThreadSafeSkipList() 
        : head(std::make_shared<Node<K, V>>(MAX_LEVEL)),
          current_level(1),
          element_count(0),
          distribution(0.0f, 1.0f) {
        // 初始化头节点
        for (int i = 0; i <= MAX_LEVEL; i++) {
            head->next[i] = nullptr;
        }
    }
    
    ~ThreadSafeSkipList() = default;
    
    // 插入键值对
    bool insert(const K& key, const V& value) {
        // 获取写锁
        std::unique_lock<std::shared_mutex> lock(rw_mutex);
        
        // 创建更新数组，记录每层的前驱节点
        std::vector<std::shared_ptr<Node<K, V>>> update(MAX_LEVEL + 1);
        std::shared_ptr<Node<K, V>> current = head;
        
        // 从最高层开始查找插入位置
        for (int i = current_level; i >= 1; i--) {
            while (current->next[i] != nullptr && current->next[i]->key < key) {
                current = current->next[i];
            }
            update[i] = current;
        }
        
        // 到达最底层
        current = current->next[1];
        
        // 如果键已存在，更新值
        if (current != nullptr && current->key == key) {
            current->value = value;
            return true;
        }
        
        // 生成新节点的随机层数
        int new_level = random_level();
        
        // 如果新节点的层数比当前最大层数高
        if (new_level > current_level) {
            for (int i = current_level + 1; i <= new_level; i++) {
                update[i] = head;
            }
            current_level = new_level;
        }
        
        // 创建新节点
        std::shared_ptr<Node<K, V>> new_node = std::make_shared<Node<K, V>>(key, value, new_level);
        
        // 插入新节点
        for (int i = 1; i <= new_level; i++) {
            new_node->next[i] = update[i]->next[i];
            update[i]->next[i] = new_node;
        }
        
        element_count++;
        return true;
    }
    
    // 查找键值对
    bool search(const K& key, V& value) {
        // 获取读锁
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        
        std::shared_ptr<Node<K, V>> current = head;
        
        // 从最高层开始查找
        for (int i = current_level; i >= 1; i--) {
            while (current->next[i] != nullptr && current->next[i]->key < key) {
                current = current->next[i];
            }
        }
        
        // 到达最底层
        current = current->next[1];
        
        if (current != nullptr && current->key == key) {
            value = current->value;
            return true;
        }
        
        return false;
    }
    
    // 删除键值对
    bool remove(const K& key) {
        // 获取写锁
        std::unique_lock<std::shared_mutex> lock(rw_mutex);
        
        std::vector<std::shared_ptr<Node<K, V>>> update(MAX_LEVEL + 1, nullptr);
        std::shared_ptr<Node<K, V>> current = head;
        
        // 从最高层开始查找要删除的节点
        for (int i = current_level; i >= 1; i--) {
            while (current->next[i] != nullptr && current->next[i]->key < key) {
                current = current->next[i];
            }
            update[i] = current;
        }
        
        // 到达最底层
        current = current->next[1];
        
        // 如果节点不存在
        if (current == nullptr || current->key != key) {
            return false;
        }
        
        // 更新前驱节点的指针
        for (int i = 1; i <= current_level; i++) {
            if (update[i]->next[i] != current) {
                break;
            }
            update[i]->next[i] = current->next[i];
        }
        
        // 降低跳表层数
        while (current_level > 1 && head->next[current_level] == nullptr) {
            current_level--;
        }
        
        element_count--;
        return true;
    }
    
    // 获取元素数量
    int size() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        return element_count;
    }
    
    // 判断是否为空
    bool empty() const {
        return size() == 0;
    }
    
    // 打印跳表（调试用）
    void display() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        
        std::cout << "\n====== Skip List (Level: " << current_level 
                  << ", Size: " << element_count << ") ======" << std::endl;
        
        for (int i = current_level; i >= 1; i--) {
            std::cout << "Level " << i << ": ";
            std::shared_ptr<Node<K, V>> node = head->next[i];
            
            while (node != nullptr) {
                std::cout << node->key << ":" << node->value << " ";
                node = node->next[i];
            }
            std::cout << std::endl;
        }
    }
    
    // 获取所有键值对
    std::vector<std::pair<K, V>> get_all() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        
        std::vector<std::pair<K, V>> result;
        std::shared_ptr<Node<K, V>> current = head->next[1];
        
        while (current != nullptr) {
            result.emplace_back(current->key, current->value);
            current = current->next[1];
        }
        
        return result;
    }
    
    // 范围查询
    std::vector<std::pair<K, V>> range(const K& start, const K& end) const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        
        std::vector<std::pair<K, V>> result;
        std::shared_ptr<Node<K, V>> current = head;
        
        // 从最高层开始查找起点
        for (int i = current_level; i >= 1; i--) {
            while (current->next[i] != nullptr && current->next[i]->key < start) {
                current = current->next[i];
            }
        }
        
        // 到达最底层
        current = current->next[1];
        
        // 收集范围内的元素
        while (current != nullptr && current->key <= end) {
            result.emplace_back(current->key, current->value);
            current = current->next[1];
        }
        
        return result;
    }
    
    // 清空跳表
    void clear() {
        std::unique_lock<std::shared_mutex> lock(rw_mutex);
        
        for (int i = 0; i <= MAX_LEVEL; i++) {
            head->next[i] = nullptr;
        }
        current_level = 1;
        element_count = 0;
    }
};

// 测试函数
void test_basic_operations() {
    std::cout << "=== 基本操作测试 ===" << std::endl;
    
    ThreadSafeSkipList<int, std::string> skiplist;
    
    // 测试插入
    skiplist.insert(3, "Apple");
    skiplist.insert(1, "Banana");
    skiplist.insert(5, "Orange");
    skiplist.insert(2, "Grape");
    skiplist.insert(4, "Pear");
    
    std::cout << "插入5个元素后：" << std::endl;
    skiplist.display();
    
    // 测试查找
    std::string value;
    if (skiplist.search(3, value)) {
        std::cout << "查找键3: " << value << std::endl;
    }
    
    // 测试范围查询
    auto range_result = skiplist.range(2, 4);
    std::cout << "范围查询[2,4]: ";
    for (const auto& pair : range_result) {
        std::cout << pair.first << ":" << pair.second << " ";
    }
    std::cout << std::endl;
    
    // 测试删除
    skiplist.remove(3);
    std::cout << "\n删除键3后：" << std::endl;
    skiplist.display();
    
    // 获取所有元素
    auto all_items = skiplist.get_all();
    std::cout << "所有元素: ";
    for (const auto& pair : all_items) {
        std::cout << pair.first << ":" << pair.second << " ";
    }
    std::cout << std::endl;
}

// 多线程测试
void test_concurrent_operations() {
    std::cout << "\n=== 多线程并发测试 ===" << std::endl;
    
    ThreadSafeSkipList<int, int> skiplist;
    const int NUM_THREADS = 4;
    const int OPS_PER_THREAD = 1000;
    
    std::vector<std::thread> threads;
    std::atomic<int> inserts_completed={0};
    std::atomic<int> searches_completed={0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建写入线程
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                int key = t * OPS_PER_THREAD + i;
                skiplist.insert(key, key * 10);
                inserts_completed++;
            }
        });
    }
    
    // 等待写入线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    threads.clear();
    
    // 创建混合操作线程
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([&, t]() {
            std::mt19937 gen(t + 42);
            std::uniform_int_distribution<> dis(0, 2);
            std::uniform_int_distribution<> key_dis(0, NUM_THREADS * OPS_PER_THREAD - 1);
            
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                int op = dis(gen);
                int key = key_dis(gen);
                
                if (op == 0) {  // 插入
                    skiplist.insert(key, key * 20);
                    inserts_completed++;
                } else if (op == 1) {  // 查找
                    int value;
                    if (skiplist.search(key, value)) {
                        searches_completed++;
                    }
                } else {  // 删除
                    skiplist.remove(key);
                }
            }
        });
    }
    
    // 等待混合操作线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "并发测试完成！" << std::endl;
    std::cout << "总时间: " << duration.count() << "ms" << std::endl;
    std::cout << "插入操作: " << inserts_completed << " 次" << std::endl;
    std::cout << "查找成功: " << searches_completed << " 次" << std::endl;
    std::cout << "最终大小: " << skiplist.size() << std::endl;
    
    // 验证数据一致性
    std::cout << "\n验证数据一致性..." << std::endl;
    bool all_correct = true;
    auto all_items = skiplist.get_all();
    
    for (const auto& pair : all_items) {
        int value;
        if (skiplist.search(pair.first, value)) {
            if (value != pair.second) {
                std::cout << "数据不一致: 键" << pair.first 
                          << " 期望值" << pair.second 
                          << " 实际值" << value << std::endl;
                all_correct = false;
            }
        } else {
            std::cout << "键不存在: " << pair.first << std::endl;
            all_correct = false;
        }
    }
    
    if (all_correct) {
        std::cout << "✓ 所有数据一致性验证通过！" << std::endl;
    } else {
        std::cout << "✗ 数据一致性验证失败！" << std::endl;
    }
}

// 性能测试
void test_performance() {
    std::cout << "\n=== 性能测试 ===" << std::endl;
    
    const int NUM_OPERATIONS = 10000;
    ThreadSafeSkipList<int, int> skiplist;
    
    // 插入性能测试
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        skiplist.insert(i, i * 2);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "插入 " << NUM_OPERATIONS << " 个元素: " 
              << insert_time.count() << "ms" << std::endl;
    
    // 查找性能测试
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        int value;
        skiplist.search(i, value);
    }
    end = std::chrono::high_resolution_clock::now();
    auto search_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "查找 " << NUM_OPERATIONS << " 个元素: " 
              << search_time.count() << "ms" << std::endl;
    
    // 删除性能测试
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        skiplist.remove(i);
    }
    end = std::chrono::high_resolution_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "删除 " << NUM_OPERATIONS << " 个元素: " 
              << delete_time.count() << "ms" << std::endl;
}

int main() {
    std::cout << "=== 线程安全跳表测试程序 ===" << std::endl;
    std::cout << "编译选项: C++17, 使用shared_mutex实现读写锁" << std::endl;
    
    // 运行测试
    test_basic_operations();
    test_concurrent_operations();
    test_performance();
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}

#endif

#if 0
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <thread>
#include <vector>

template<class K,class V>
class Node{
private:
  K key;
  V value;
  std::vector<std::shared_ptr<Node<K,V>>next;
  std::mutex mtx;
  int level;
public:
  Node(K k,V v,int l):key(k),value(v),level(l){
    next.resize(l+1,nullptr);
  }

  Node(int l):key(K()),value(V()),level(l){
    next.resize(level+1,nullptr);
  }
};


template<class K,class V>
class SkipList{
private:
  const int max_level=16;
  const float P=0.5f;
  std::shared_ptr<Node<K,V>>head;
  int current_level;
  int element_count;
  mutable shared_mutex rw_mutex;
  std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution;
  
  int random_level(){
    int level=1;
    while(distribution(generator)<P && level<max_level){
        level++;
    }
    return level;
  }

public:
  
  SkipList():head(std::make_shared<Node<K,V>>(max_level)),current_level(1),element_count(0),distribution(0.0f,1.0f){
    for(int i=0;i<=max_level;i++){
      head[i]->next=nullptr;
    }
  }

  ~SkipList()=default;


  bool insert(const K& key,const V& value){
    std::unique_lock<mutex>lock(rw_mutex);
    std::vector<std::shared_ptr<Node<K,V>>>update(max_level+1);
    std::shared_ptr<Node<K,V>>current=head;
    
    for(int i=current_level;i>=1;i--){
      while(current->level[i]!=nullptr && current->next[i]->key<key){
        current=current->next[i];
      }
      update[i]=current;
    }

    current=current->next[1];

    if(current!=nullptr && current->key==key){
        current->value=value;
        return true;
    }

    int new_level=random_level();
    if(new_level>current_level){
        for(int i=current_level;i<new_level;i++){
            update[i]=head;
        }
        current_level=new_level;
    }
    std::shared_ptr<Node<K,V>>new_node=std::make_shared<Node<K,V>>(key,value,new_level);

    for(int i=1;i<=new_level;i++){
      new_node->next[i]=update[i]->next[i];
      update[i]->next=new_node;
    }

    element_count++;
    return true;
  }

  bool search(const K& key,V& value){
     std::shared_lock<std::shared_mutex>lock(rw_muex);
     std::shared_ptr<Node<K,V>>current=head;
     for(int i=current_level;i>=1;i--){
        while(current->next[i]!=nullptr && current->next[i]->key<key){
            current=current->next[i];
        }
     }

     current=current->next[1];
     if(current!=nullptr && current->key==key){
          value=current->value;
          return true;
     }

     return false;
  }
  

  bool remove(const K& key){
      std::unique_lock<std::shared_mutex>lock(rw_mutex);
      std::vector<std::shared_ptr<Node<K,V>>update(max_level+1,nullptr);
      std::shared_ptr<Node<K,V>>current=head;
      for(int i=current_level;i>=1;i--){
          while(current->next[i]!=nullptr && current->next[i]->key<key){
              current=current->next[i];
          }
          update[i]=current;
      }

      current=current->next[1];
      if(current==nullptr || current->key!=key){
          return false;
      }

      for(int i=1;i<=current_level;i++){
          if(update[i]->next[i]!=current){
            break;
          }
          update[i]->next[i]=current->next[i];
      }

      while(current_level >1 && head->next[current_level]==nullptr){
          current_level--;
      }

      elment_count--;
      return true;
  }
};
#endif

#include <iostream>
using namespace std;
template <class K, class V>
class Node {
 public:
  vector < shared_ptr<Node<K, V>> next;
  shared_lock rwmtx;
  K key;
  V value;
  int level;
  Node(K v, V v, int l) : key(k), value(v), level(l) {
    next.resize(level + 1, nullptr);
  }

  Node(int l) : key(K()), value(V()), level(l) {
    next.resize(level + 1, nullptr);
  }
};

template <class K, class V>
class SkipList {
 private:
  shared_ptr<Node<K, V>> head;
  const int max_level = 16;
  const float p = 0.5f;
  int current_level;
  mutable shared_mutex rw_mtx;
  std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution;
  int random_level() {
    int level = 1;
    while (distribution(generator) < P && level < MAX_LEVEL) {
      level++;
    }
    return level;
  }

 public:
  SkipList()
      : head(make_shared<Node<K, V>>(max_level)),
        current_level(1),
        element_count(0),
        distribution(0.0f, 1.0f) {
    for (int i = 0; i <= max_level; i++) {
      head->next[i] = nullptr;
    }
  }

  ~SkipList() = default;

  bool insert(const K& key, const V& value) {
    std::unique_lock<shared_mutex> lock(rw_mutx);
    std::vector < std::shared_ptr<Node<K, V>> update(max_level + 1);
    std::shared_ptr<Node<K, V>> current = head;
    for (int i = current_level; i >= 1; i--) {
      while (current->next[i] != nullptr && current->next[i]->key < key) {
        current = current->next[i];
      }
      update[i] = current;
    }

    current = current->next[1];
    if (current != nullptr && current->key == key) {
      current->value == value;
      return true;
    }

    int new_level = random_level();
    if (new_level > current_level) {
      for (int i = current_level; i <= new_level; i++) {
        update[i] = head;
      }
      current_level = new_level;
    }

    shared_ptr<Node<K, V>> new_node =
        make_shared<Node<K, V>>(key, value, new_level);
    for (int i = 1; i < current_level; i++) {
      new_node->next = update[i]->next[i];
      update[i]->next[i] = new_node;
    }

    element_count++;
    return true;
  }

  bool get(const K& k, V& value) {
    shared_lock<shared_mtx> lokc(rw_mtx);
    shared_ptr<Node<K, V>> current = head;
    for (int i = current_level; i >= 1; i--) {
      if (current->next[i] != nullptr && current->next[i]->key < key) {
        current = current->next[i];
      }
    }

    current = current->next[1];

    if (current != nullptr && current->key == key) {
      value = current->value;
      return true;
    }
    return false;
  }

  bool remove(const K& key) {
    unique_lock<shared_mutex> lock(rw_mtx);
    shared_ptr<Node<K, V>> current = head;
    vector < shared_ptr<Node<K, V>> update(max_level + 1, nullptr);

    for (int i = current_level; i >= 1; i--) {
      while (current->next[i] != nullptr && current->next[i]->key < key) {
        current = current->next[i];
      }
      update[i] = current;
    }

    current = current->next[1];

    if (current == nullptr || current->key != key) return false;
    ;
    for (int i = 1; i < current_level; i++) {
      if (update[i]->next[i] != current) {
        continue;
      }

      update[i]->next[i] = current->next[i];
    }

    while (current_level > 1 && head->next[current_level] == nullptr) {
      current_level--;
    }
    element_count--;
    return true;
  }
};
int main() { return 0; }




#include<iostream>
using namespace std;
template<class K,class V>
class Node{
  K key;
  V value;
  int current_level;
  vector<shared_ptr<Node<K,V>>>next;
  Node(K k,V v,int level):key(k),value(v),current_level(level){
    next.resize(current_level+1,nullptr);
  }

  Node(int level):key(K()),value(V()),level(current_level){
    next.resize(current_level+1,nullptr);
  }
};
class SkipList{
  int max_level;
  int element_level;
  shared_ptr<Node<K,V>>head; 
};

SkipList():max_level(1),element_count(0),head(make_shared<Node<K,V>>(max_level){
  for(int i=0;i<max_level;i++){
    head->next[i]=nullptr;
  }
}

~SkipList()=default;


template<class K,class V>
bool insert(const K& key,const V& value){
  lock_guard<mutex>lock(mtx);
  shared_ptr<Node<K,V>>current=head;
  vector<shared_ptr<Node<K,V>>>update(max_level+1,nullptr);
  for(int i=0;i<current_level;i++)){
    while(current->next[i]!=nullptr && current->next[i]->key < key){
      current=current->next[i];
    }  
    update[i]=current;
  }
  current=curretn->next[1];
  
  if(current!=nullptr || current->key == key){
    current->value=value;
    return true;
  }
  
  int new_level=random_level();
  
  if(new_level > current_level){
    for(int i=current_level;i<new_level;i++){
      head[i]=nullptr;
    }
    current_level=new_level;
  }
  shared_ptr<Node<K,V>>new_node=make_shared<Node{key,value,new_level});
  for(int i=0;i<current_level;i++){
    new_node->next[i]=current->next[i];
    update[i]->next[i]=new_node;
    element_count++;
  }
  return true;
}
template<class K,class V>
bool remove(const K& key){
  lock_guard<mutex>lock(mtx);
  shared_ptr<Node<K,V>>current=head;
  vector<shared_ptr<Node<K,V>>update(max_level+1,nullptr);
  for(int i=0;i<current_level;i++){
    while(current->next[i]!=nullptr && current->next[i]->key < key){
      current=current->next[i];
    }
    update[i]=current;
  }

  current=current->next[1];
  
  if(current==nullptr || current->key!=key){
    return false;
  }
  
  for(int i=0;i<current_level;i++){
    if(current->next[i]==nullptr)continue;
    update[i]->next[i]=current->next[i];
  }
  if(current_level>1 && head->next[i]==nullptr){
    current_level--;
  }
  elment_count--;
  return true;
}
int main(){
  
}
















