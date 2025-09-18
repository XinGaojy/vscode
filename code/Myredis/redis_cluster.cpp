#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstring>
#include <random>
#include <functional>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sched.h>
#include <errno.h>
#include <stdexcept>
#include <queue>
#include <signal.h>

// 集群配置常量
constexpr int CLUSTER_SLOTS = 16384;
constexpr int CLUSTER_PORT = 7000;
constexpr int CLUSTER_REPLICAS = 1;
constexpr int CLUSTER_NODE_TIMEOUT = 5000; // 5秒
constexpr int CLUSTER_FAILOVER_TIMEOUT = 10000; // 10秒
constexpr int CLUSTER_MIGRATION_TIMEOUT = 30000; // 30秒
constexpr int CLUSTER_GOSSIP_INTERVAL = 1000; // 1秒

// 协程框架常量
constexpr size_t MAX_EVENTS = 64;
constexpr size_t MAX_COROUTINES = 10000;
constexpr size_t STACK_SIZE = 64 * 1024; // 64KB
constexpr size_t MAX_SCHEDULERS = 8;

namespace RedisCluster {

/* ================================================================== */
/*                          集群数据结构定义                           */
/* ================================================================== */

// 集群节点状态
enum class NodeState {
    CONNECTING,   // 正在连接
    CONNECTED,    // 已连接
    FAIL,         // 节点失效
    HANDSHAKE,    // 握手状态
    NOADDR,       // 无地址
    MEET,         // MEET消息已发送
    PFAIL,        // 可能失效
    FAIL_REPORT,  // 失效报告
    FORGET,       // 被遗忘
    LEAVING,      // 正在离开
    MIGRATING,    // 正在迁移
    IMPORTING     // 正在导入
};

// 集群节点角色
enum class NodeRole {
    MASTER,       // 主节点
    REPLICA,      // 从节点
    UNKNOWN       // 未知
};

// 集群槽位状态
enum class SlotState {
    STABLE,       // 稳定状态
    MIGRATING,    // 迁移中
    IMPORTING     // 导入中
};

// 集群节点信息
struct ClusterNode {
    std::string id;            // 节点ID
    std::string ip;            // IP地址
    int port;                  // 端口
    NodeRole role;             // 节点角色
    NodeState state;           // 节点状态
    std::set<int> slots;       // 负责的槽位
    std::vector<std::string> replicas; // 从节点ID列表
    std::chrono::steady_clock::time_point last_ping; // 最后ping时间
    std::chrono::steady_clock::time_point last_pong; // 最后pong时间
    int fd;                    // 连接套接字
    bool myself;               // 是否为本节点
};

// 集群槽位信息
struct ClusterSlot {
    int slot;                  // 槽位编号
    SlotState state;           // 槽位状态
    std::string migrating_to;  // 迁移目标节点ID
    std::string importing_from; // 导入来源节点ID
};

// 集群配置
struct ClusterConfig {
    std::string current_epoch; // 当前配置纪元
    std::map<std::string, ClusterNode> nodes; // 所有节点
    std::map<int, ClusterSlot> slots; // 所有槽位
    std::string myself_id;     // 本节点ID
    int myself_port;           // 本节点端口
    std::string myself_ip;     // 本节点IP
};

/* ================================================================== */
/*                          协程Reactor框架                           */
/* ================================================================== */

// 上下文结构
struct context { 
    void* rsp; 
    void* rbp;
    void* rbx;
    void* r12;
    void* r13;
    void* r14;
    void* r15;
};

#if defined(__x86_64__)
static inline void ctx_switch(context* old_, context* new_) {
    asm volatile(
        "pushq %%rbp\n\t"
        "pushq %%rbx\n\t"
        "pushq %%r12\n\t"
        "pushq %%r13\n\t"
        "pushq %%r14\n\t"
        "pushq %%r15\n\t"
        "movq %%rsp, %0\n\t"
        "movq %%rbp, %1\n\t"
        "movq %%rbx, %2\n\t"
        "movq %%r12, %3\n\t"
        "movq %%r13, %4\n\t"
        "movq %%r14, %5\n\t"
        "movq %%r15, %6\n\t"
        "movq %7, %%rsp\n\t"
        "movq %8, %%rbp\n\t"
        "movq %9, %%rbx\n\t"
        "movq %10, %%r12\n\t"
        "movq %11, %%r13\n\t"
        "movq %12, %%r14\n\t"
        "movq %13, %%r15\n\t"
        "popq %%r15\n\t"
        "popq %%r14\n\t"
        "popq %%r13\n\t"
        "popq %%r12\n\t"
        "popq %%rbx\n\t"
        "popq %%rbp\n\t"
        "ret"
        : "=m"(old_->rsp), "=m"(old_->rbp), "=m"(old_->rbx),
          "=m"(old_->r12), "=m"(old_->r13), "=m"(old_->r14), "=m"(old_->r15)
        : "m"(new_->rsp), "m"(new_->rbp), "m"(new_->rbx),
          "m"(new_->r12), "m"(new_->r13), "m"(new_->r14), "m"(new_->r15)
        : "memory");
}
#endif

namespace Reactor {

/* ------------------------------------------------------------------ */
/*  协程对象                                                           */
/* ------------------------------------------------------------------ */
class Coroutine {
public:
    using Func = std::function<void()>;
    enum State { READY, RUNNING, WAITING, FINISHED };
    
    Coroutine(Func func, size_t stack_size = STACK_SIZE);
    ~Coroutine();
    
    void resume();
    void yield();
    State state() const { return state_; }
    
private:
    static void trampoline(void* arg);
    
    Func func_;
    State state_;
    context context_;
    char* stack_;
    size_t stack_size_;
};

/* ------------------------------------------------------------------ */
/*  调度器                                                             */
/* ------------------------------------------------------------------ */
class Scheduler {
public:
    Scheduler();
    ~Scheduler();
    
    void spawn(Coroutine::Func func);
    void run();
    void stop();
    
    void wait_fd(int fd, int events);
    void register_timer(std::function<void()> callback, int interval_ms);
    
    static Scheduler* instance();
    
private:
    void io_worker();
    void timer_worker();
    
    int epoll_fd_;
    int timer_fd_;
    std::thread io_thread_;
    std::thread timer_thread_;
    std::atomic<bool> running_;
    std::queue<Coroutine*> ready_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::map<int, Coroutine*> waiting_coros_;
    std::mutex io_mutex_;
    std::map<int, std::function<void()>> timers_;
    std::mutex timer_mutex_;
};

} // namespace Reactor

/* ================================================================== */
/*                          Redis存储引擎                             */
/* ================================================================== */

class RedisStorage {
public:
    // 存储类型
    enum class ValueType {
        STRING,
        LIST,
        SET,
        HASH,
        ZSET,
        NONE
    };
    
    // 存储值
    struct Value {
        ValueType type;
        std::string data;
        std::chrono::steady_clock::time_point expire_time;
    };
    
    // 设置键值对
    void set(const std::string& key, const std::string& value, 
             long long ttl_ms = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        Value v;
        v.type = ValueType::STRING;
        v.data = value;
        if (ttl_ms > 0) {
            v.expire_time = std::chrono::steady_clock::now() + 
                           std::chrono::milliseconds(ttl_ms);
        }
        data_[key] = v;
    }
    
    // 获取键值对
    bool get(const std::string& key, std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it == data_.end()) return false;
        
        // 检查过期时间
        if (it->second.expire_time.time_since_epoch().count() > 0 &&
            it->second.expire_time < std::chrono::steady_clock::now()) {
            data_.erase(it);
            return false;
        }
        
        value = it->second.data;
        return true;
    }
    
    // 删除键值对
    bool del(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.erase(key) > 0;
    }
    
    // 检查键是否存在
    bool exists(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        if (it == data_.end()) return false;
        
        // 检查过期时间
        if (it->second.expire_time.time_since_epoch().count() > 0 &&
            it->second.expire_time < std::chrono::steady_clock::now()) {
            data_.erase(it);
            return false;
        }
        
        return true;
    }
    
    // 清理过期键
    void cleanup_expired() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        for (auto it = data_.begin(); it != data_.end();) {
            if (it->second.expire_time.time_since_epoch().count() > 0 &&
                it->second.expire_time < now) {
                it = data_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    std::unordered_map<std::string, Value> data_;
    std::mutex mutex_;
};

/* ================================================================== */
/*                          Redis集群实现                              */
/* ================================================================== */

class RedisCluster {
public:
    RedisCluster(const std::string& ip, int port, 
                 const std::vector<std::string>& seed_nodes);
    
    // 启动集群
    void start();
    
    // 处理客户端请求
    void handle_client(int fd);
    
    // 集群命令处理
    void process_command(int fd, const std::vector<std::string>& args);
    
private:
    // 集群初始化
    void initialize_cluster();
    
    // 节点发现
    void discover_nodes();
    
    // 槽分配
    void assign_slots();
    
    // 心跳检测
    void heartbeat();
    
    // 故障检测
    void failure_detection();
    
    // 故障转移
    void failover();
    
    // 槽迁移
    void migrate_slot(int slot, const std::string& target_node_id);
    
    // 处理集群命令
    void process_cluster_command(int fd, const std::vector<std::string>& args);
    
    // 处理数据命令
    void process_data_command(int fd, const std::vector<std::string>& args);
    
    // 计算键的槽位
    int key_to_slot(const std::string& key);
    
    // 获取负责槽位的节点
    std::string get_node_for_slot(int slot);
    
    // 发送重定向
    void send_redirect(int fd, int slot, const std::string& node_id);
    
    // 发送错误
    void send_error(int fd, const std::string& error);
    
    // 发送响应
    void send_response(int fd, const std::string& response);
    
    // 节点间通信
    void send_to_node(const std::string& node_id, const std::string& message);
    
    // 生成节点ID
    std::string generate_node_id();
    
    // 集群配置
    ClusterConfig config_;
    
    // 存储引擎
    RedisStorage storage_;
    
    // 协程调度器
    Reactor::Scheduler scheduler_;
    
    // 监听套接字
    int listen_fd_;
    
    // 节点间通信套接字
    std::map<std::string, int> node_sockets_;
    
    // 随机数生成器
    std::mt19937 rng_;
};

/* ================================================================== */
/*                          Redis集群实现细节                          */
/* ================================================================== */

RedisCluster::RedisCluster(const std::string& ip, int port, 
                           const std::vector<std::string>& seed_nodes)
    : config_(), rng_(std::random_device{}()) {
    
    // 设置本节点信息
    config_.myself_ip = ip;
    config_.myself_port = port;
    config_.myself_id = generate_node_id();
    
    // 添加本节点
    ClusterNode myself;
    myself.id = config_.myself_id;
    myself.ip = ip;
    myself.port = port;
    myself.role = NodeRole::MASTER;
    myself.state = NodeState::CONNECTED;
    myself.myself = true;
    myself.last_pong = std::chrono::steady_clock::now();
    config_.nodes[myself.id] = myself;
    
    // 添加种子节点
    for (const auto& seed : seed_nodes) {
        size_t pos = seed.find(':');
        if (pos == std::string::npos) continue;
        
        std::string seed_ip = seed.substr(0, pos);
        int seed_port = std::stoi(seed.substr(pos + 1));
        
        ClusterNode node;
        node.id = "unknown"; // 初始ID未知
        node.ip = seed_ip;
        node.port = seed_port;
        node.role = NodeRole::UNKNOWN;
        node.state = NodeState::CONNECTING;
        node.myself = false;
        config_.nodes[node.id] = node;
    }
    
    // 创建监听套接字
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(listen_fd_);
        throw std::runtime_error("Failed to bind socket");
    }
    
    if (listen(listen_fd_, 128) < 0) {
        close(listen_fd_);
        throw std::runtime_error("Failed to listen on socket");
    }
}

void RedisCluster::start() {
    // 初始化集群
    initialize_cluster();
    
    // 启动心跳检测
    scheduler_.spawn([this] {
        while (true) {
            heartbeat();
            Reactor::Scheduler::instance()->wait_fd(-1, 0); // 等待1秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    // 启动故障检测
    scheduler_.spawn([this] {
        while (true) {
            failure_detection();
            Reactor::Scheduler::instance()->wait_fd(-1, 0); // 等待1秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    // 启动过期键清理
    scheduler_.spawn([this] {
        while (true) {
            storage_.cleanup_expired();
            Reactor::Scheduler::instance()->wait_fd(-1, 0); // 等待5秒
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    
    // 接受客户端连接
    scheduler_.spawn([this] {
        while (true) {
            sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    Reactor::Scheduler::instance()->wait_fd(listen_fd_, EPOLLIN);
                    continue;
                }
                perror("accept");
                continue;
            }
            
            // 设置非阻塞
            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            
            // 处理客户端
            scheduler_.spawn([this, client_fd] {
                handle_client(client_fd);
            });
        }
    });
    
    // 运行调度器
    scheduler_.run();
}

void RedisCluster::handle_client(int fd) {
    char buffer[4096];
    
    while (true) {
        // 等待数据可读
        scheduler_.wait_fd(fd, EPOLLIN);
        
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            close(fd);
            return;
        }
        
        // 解析命令
        std::vector<std::string> args;
        std::istringstream iss(std::string(buffer, n));
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }
        
        if (args.empty()) continue;
        
        // 处理命令
        process_command(fd, args);
    }
}

void RedisCluster::process_command(int fd, const std::vector<std::string>& args) {
    if (args[0] == "CLUSTER") {
        process_cluster_command(fd, args);
    } else {
        process_data_command(fd, args);
    }
}

void RedisCluster::initialize_cluster() {
    // 发现节点
    discover_nodes();
    
    // 分配槽位
    assign_slots();
    
    // 建立节点间连接
    for (auto& pair : config_.nodes) {
        if (pair.second.id == config_.myself_id) continue;
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(pair.second.port);
        inet_pton(AF_INET, pair.second.ip.c_str(), &addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            continue;
        }
        
        // 设置非阻塞
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        
        node_sockets_[pair.second.id] = sock;
        
        // 发送MEET消息
        std::string meet_msg = "CLUSTER MEET " + config_.myself_ip + " " + 
                              std::to_string(config_.myself_port);
        send(sock, meet_msg.c_str(), meet_msg.size(), 0);
    }
}

void RedisCluster::discover_nodes() {
    // 从种子节点获取集群信息
    for (auto& pair : config_.nodes) {
        if (pair.second.id == config_.myself_id) continue;
        
        // 发送CLUSTER NODES命令
        std::string nodes_cmd = "CLUSTER NODES\r\n";
        if (node_sockets_.find(pair.second.id) != node_sockets_.end()) {
            int sock = node_sockets_[pair.second.id];
            send(sock, nodes_cmd.c_str(), nodes_cmd.size(), 0);
            
            // 接收响应
            char buffer[4096];
            ssize_t n = recv(sock, buffer, sizeof(buffer), 0);
            if (n > 0) {
                std::string response(buffer, n);
                // 解析节点信息
                // 格式: <id> <ip:port> <flags> <master> <ping-sent> <pong-recv> <config-epoch> <link-state> <slots>
                std::istringstream iss(response);
                std::string line;
                while (std::getline(iss, line)) {
                    std::istringstream line_iss(line);
                    std::string id, addr, flags, master, ping, pong, epoch, state, slots;
                    line_iss >> id >> addr >> flags >> master >> ping >> pong >> epoch >> state;
                    
                    // 解析IP和端口
                    size_t pos = addr.find(':');
                    if (pos == std::string::npos) continue;
                    std::string ip = addr.substr(0, pos);
                    int port = std::stoi(addr.substr(pos + 1));
                    
                    // 添加节点
                    if (config_.nodes.find(id) == config_.nodes.end()) {
                        ClusterNode node;
                        node.id = id;
                        node.ip = ip;
                        node.port = port;
                        node.role = flags.find("master") != std::string::npos ? 
                                   NodeRole::MASTER : NodeRole::REPLICA;
                        node.state = NodeState::CONNECTED;
                        node.myself = false;
                        config_.nodes[id] = node;
                    }
                    
                    // 解析槽位
                    while (line_iss >> slots) {
                        if (slots.find('[') != std::string::npos) continue;
                        if (slots.find('-') != std::string::npos) {
                            size_t dash = slots.find('-');
                            int start = std::stoi(slots.substr(0, dash));
                            int end = std::stoi(slots.substr(dash + 1));
                            for (int i = start; i <= end; i++) {
                                config_.nodes[id].slots.insert(i);
                                config_.slots[i].slot = i;
                                config_.slots[i].state = SlotState::STABLE;
                            }
                        } else {
                            int slot = std::stoi(slots);
                            config_.nodes[id].slots.insert(slot);
                            config_.slots[slot].slot = slot;
                            config_.slots[slot].state = SlotState::STABLE;
                        }
                    }
                }
            }
        }
    }
}

void RedisCluster::assign_slots() {
    // 如果没有槽位分配，则分配槽位
    if (config_.nodes[config_.myself_id].slots.empty()) {
        // 计算每个主节点应该负责的槽位数
        int num_masters = 0;
        for (const auto& pair : config_.nodes) {
            if (pair.second.role == NodeRole::MASTER) {
                num_masters++;
            }
        }
        
        if (num_masters == 0) return;
        
        int slots_per_master = CLUSTER_SLOTS / num_masters;
        int extra_slots = CLUSTER_SLOTS % num_masters;
        
        // 分配槽位
        int slot = 0;
        for (auto& pair : config_.nodes) {
            if (pair.second.role != NodeRole::MASTER) continue;
            
            int slots_to_assign = slots_per_master;
            if (extra_slots > 0) {
                slots_to_assign++;
                extra_slots--;
            }
            
            for (int i = 0; i < slots_to_assign; i++) {
                pair.second.slots.insert(slot);
                config_.slots[slot].slot = slot;
                config_.slots[slot].state = SlotState::STABLE;
                slot++;
            }
        }
    }
}

void RedisCluster::heartbeat() {
    auto now = std::chrono::steady_clock::now();
    
    // 发送PING给所有节点
    for (auto& pair : config_.nodes) {
        if (pair.second.id == config_.myself_id) continue;
        
        if (node_sockets_.find(pair.second.id) != node_sockets_.end()) {
            int sock = node_sockets_[pair.second.id];
            std::string ping_msg = "PING\r\n";
            send(sock, ping_msg.c_str(), ping_msg.size(), 0);
            pair.second.last_ping = now;
        }
    }
    
    // 检查超时节点
    for (auto& pair : config_.nodes) {
        if (pair.second.id == config_.myself_id) continue;
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pair.second.last_pong);
        
        if (duration.count() > CLUSTER_NODE_TIMEOUT) {
            pair.second.state = NodeState::PFAIL;
        }
    }
}

void RedisCluster::failure_detection() {
    // 检查是否有主节点失效
    for (auto& pair : config_.nodes) {
        if (pair.second.role != NodeRole::MASTER) continue;
        if (pair.second.state != NodeState::PFAIL) continue;
        
        // 检查是否超过故障转移超时
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pair.second.last_pong);
        
        if (duration.count() > CLUSTER_FAILOVER_TIMEOUT) {
            pair.second.state = NodeState::FAIL;
            failover();
        }
    }
}

void RedisCluster::failover() {
    // 寻找失效的主节点
    std::string failed_master_id;
    for (const auto& pair : config_.nodes) {
        if (pair.second.role == NodeRole::MASTER && 
            pair.second.state == NodeState::FAIL) {
            failed_master_id = pair.first;
            break;
        }
    }
    
    if (failed_master_id.empty()) return;
    
    // 寻找该主节点的从节点
    std::vector<std::string> replicas;
    for (const auto& pair : config_.nodes) {
        if (pair.second.role == NodeRole::REPLICA && 
            std::find(pair.second.replicas.begin(), pair.second.replicas.end(), 
                     failed_master_id) != pair.second.replicas.end()) {
            replicas.push_back(pair.first);
        }
    }
    
    if (replicas.empty()) return;
    
    // 选举新的主节点（简化版：选择第一个从节点）
    std::string new_master_id = replicas[0];
    
    // 提升从节点为主节点
    config_.nodes[new_master_id].role = NodeRole::MASTER;
    
    // 转移槽位
    for (int slot : config_.nodes[failed_master_id].slots) {
        config_.nodes[new_master_id].slots.insert(slot);
        config_.slots[slot].state = SlotState::STABLE;
    }
    config_.nodes[failed_master_id].slots.clear();
    
    // 更新配置纪元
    config_.current_epoch = std::to_string(std::stoi(config_.current_epoch) + 1);
    
    // 广播故障转移信息
    for (auto& pair : config_.nodes) {
        if (pair.second.id == config_.myself_id) continue;
        
        if (node_sockets_.find(pair.second.id) != node_sockets_.end()) {
            int sock = node_sockets_[pair.second.id];
            std::string failover_msg = "CLUSTER FAILOVER " + failed_master_id + " " + new_master_id;
            send(sock, failover_msg.c_str(), failover_msg.size(), 0);
        }
    }
}

void RedisCluster::migrate_slot(int slot, const std::string& target_node_id) {
    // 设置槽位状态为迁移中
    config_.slots[slot].state = SlotState::MIGRATING;
    config_.slots[slot].migrating_to = target_node_id;
    
    // 获取目标节点套接字
    if (node_sockets_.find(target_node_id) == node_sockets_.end()) {
        return;
    }
    int target_sock = node_sockets_[target_node_id];
    
    // 迁移数据
    // 在实际实现中，这里需要遍历所有键并迁移属于该槽位的键
    // 这里简化处理，只发送迁移命令
    std::string migrate_cmd = "CLUSTER MIGRATE " + std::to_string(slot);
    send(target_sock, migrate_cmd.c_str(), migrate_cmd.size(), 0);
    
    // 等待迁移完成
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (duration.count() > CLUSTER_MIGRATION_TIMEOUT) {
            break;
        }
        
        // 检查迁移状态
        // 在实际实现中，需要与目标节点通信确认迁移完成
        // 这里简化处理，直接标记为完成
        config_.slots[slot].state = SlotState::STABLE;
        config_.slots[slot].migrating_to = "";
        config_.nodes[target_node_id].slots.insert(slot);
        config_.nodes[config_.myself_id].slots.erase(slot);
        break;
    }
}

void RedisCluster::process_cluster_command(int fd, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        send_error(fd, "ERR wrong number of arguments for 'cluster' command");
        return;
    }
    
    if (args[1] == "NODES") {
        // 返回集群节点信息
        std::string response;
        for (const auto& pair : config_.nodes) {
            const ClusterNode& node = pair.second;
            response += node.id + " " + node.ip + ":" + std::to_string(node.port) + " ";
            response += (node.role == NodeRole::MASTER) ? "master" : "slave";
            response += " " + std::to_string(node.slots.size()) + "\r\n";
        }
        send_response(fd, response);
    } else if (args[1] == "SLOTS") {
        // 返回槽位分配信息
        std::string response;
        for (int slot = 0; slot < CLUSTER_SLOTS; slot++) {
            if (config_.slots.find(slot) != config_.slots.end()) {
                const ClusterSlot& s = config_.slots[slot];
                response += std::to_string(slot) + " ";
                response += (s.state == SlotState::STABLE) ? "stable" : 
                           (s.state == SlotState::MIGRATING) ? "migrating" : "importing";
                response += "\r\n";
            }
        }
        send_response(fd, response);
    } else if (args[1] == "MEET") {
        if (args.size() < 4) {
            send_error(fd, "ERR wrong number of arguments for 'cluster meet'");
            return;
        }
        
        std::string ip = args[2];
        int port = std::stoi(args[3]);
        
        // 添加新节点
        std::string node_id = generate_node_id();
        ClusterNode node;
        node.id = node_id;
        node.ip = ip;
        node.port = port;
        node.role = NodeRole::UNKNOWN;
        node.state = NodeState::CONNECTING;
        node.myself = false;
        config_.nodes[node_id] = node;
        
        // 建立连接
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            send_error(fd, "ERR failed to connect to node");
            return;
        }
        
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            send_error(fd, "ERR failed to connect to node");
            return;
        }
        
        // 设置非阻塞
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        
        node_sockets_[node_id] = sock;
        
        send_response(fd, "OK");
    } else if (args[1] == "ADDSLOTS") {
        if (args.size() < 3) {
            send_error(fd, "ERR wrong number of arguments for 'cluster addslots'");
            return;
        }
        
        // 解析槽位
        std::set<int> slots;
        for (size_t i = 2; i < args.size(); i++) {
            if (args[i].find('-') != std::string::npos) {
                size_t dash = args[i].find('-');
                int start = std::stoi(args[i].substr(0, dash));
                int end = std::stoi(args[i].substr(dash + 1));
                for (int j = start; j <= end; j++) {
                    slots.insert(j);
                }
            } else {
                slots.insert(std::stoi(args[i]));
            }
        }
        
        // 分配槽位
        for (int slot : slots) {
            config_.nodes[config_.myself_id].slots.insert(slot);
            config_.slots[slot].slot = slot;
            config_.slots[slot].state = SlotState::STABLE;
        }
        
        send_response(fd, "OK");
    } else {
        send_error(fd, "ERR unknown cluster subcommand");
    }
}

void RedisCluster::process_data_command(int fd, const std::vector<std::string>& args) {
    if (args.empty()) {
        send_error(fd, "ERR empty command");
        return;
    }
    
    std::string command = args[0];
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);
    
    // 计算键的槽位
    int slot = key_to_slot(args[1]);
    
    // 检查槽位是否由本节点负责
    std::string node_id = get_node_for_slot(slot);
    if (node_id != config_.myself_id) {
        // 重定向到正确的节点
        send_redirect(fd, slot, node_id);
        return;
    }
    
    // 处理命令
    if (command == "SET") {
        if (args.size() < 3) {
            send_error(fd, "ERR wrong number of arguments for 'set' command");
            return;
        }
        
        long long ttl = 0;
        if (args.size() > 4 && args[3] == "EX") {
            ttl = std::stoll(args[4]) * 1000;
        }
        
        storage_.set(args[1], args[2], ttl);
        send_response(fd, "OK");
    } else if (command == "GET") {
        if (args.size() < 2) {
            send_error(fd, "ERR wrong number of arguments for 'get' command");
            return;
        }
        
        std::string value;
        if (storage_.get(args[1], value)) {
            send_response(fd, value);
        } else {
            send_response(fd, "(nil)");
        }
    } else if (command == "DEL") {
        if (args.size() < 2) {
            send_error(fd, "ERR wrong number of arguments for 'del' command");
            return;
        }
        
        bool deleted = storage_.del(args[1]);
        send_response(fd, deleted ? "1" : "0");
    } else {
        send_error(fd, "ERR unknown command '" + command + "'");
    }
}

int RedisCluster::key_to_slot(const std::string& key) {
    // 计算键的CRC16校验和
    const unsigned char* data = (const unsigned char*)key.c_str();
    size_t len = key.size();
    
    // CRC16算法（简化版）
    unsigned int crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ data[i];
    }
    
    // 取模得到槽位
    return crc % CLUSTER_SLOTS;
}

std::string RedisCluster::get_node_for_slot(int slot) {
    if (config_.slots.find(slot) == config_.slots.end()) {
        return "";
    }
    
    // 如果槽位正在迁移，返回目标节点
    if (config_.slots[slot].state == SlotState::MIGRATING) {
        return config_.slots[slot].migrating_to;
    }
    
    // 查找负责该槽位的节点
    for (const auto& pair : config_.nodes) {
        if (pair.second.slots.find(slot) != pair.second.slots.end()) {
            return pair.first;
        }
    }
    
    return "";
}

void RedisCluster::send_redirect(int fd, int slot, const std::string& node_id) {
    if (config_.nodes.find(node_id) == config_.nodes.end()) {
        send_error(fd, "ERR cluster down");
        return;
    }
    
    const ClusterNode& node = config_.nodes[node_id];
    std::string redirect = "-MOVED " + std::to_string(slot) + " " + 
                          node.ip + ":" + std::to_string(node.port) + "\r\n";
    send(fd, redirect.c_str(), redirect.size(), 0);
}

void RedisCluster::send_error(int fd, const std::string& error) {
    std::string response = "-" + error + "\r\n";
    send(fd, response.c_str(), response.size(), 0);
}

void RedisCluster::send_response(int fd, const std::string& response) {
    std::string formatted = "+" + response + "\r\n";
    send(fd, formatted.c_str(), formatted.size(), 0);
}

std::string RedisCluster::generate_node_id() {
    // 使用时间戳和随机数生成节点ID
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::stringstream ss;
    ss << std::hex << millis << "-" << rng_();
    return ss.str();
}

} // namespace RedisCluster

/* ================================================================== */
/*                          主函数                                     */
/* ================================================================== */

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> [seed_nodes...]\n";
        return 1;
    }
    
    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    std::vector<std::string> seed_nodes;
    
    for (int i = 3; i < argc; i++) {
        seed_nodes.push_back(argv[i]);
    }
    
    try {
        RedisCluster::RedisCluster cluster(ip, port, seed_nodes);
        cluster.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}