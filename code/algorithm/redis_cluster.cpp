#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <random>
#include <functional>
#include <atomic>
#include<mutex>
#include <condition_variable>
#include <queue>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
using namespace std;
// Constants
const int CLUSTER_SLOTS = 16384;
const int CLUSTER_PORT = 6379;
const int CLUSTER_NODE_TIMEOUT = 15000; // 15 seconds
const int CLUSTER_FAIL_REPORT_VALIDITY_MULT = 2;
const int CLUSTER_FAIL_UNDO_TIME_MULT = 2;
const int CLUSTER_MF_TIMEOUT = 5000; // 5 seconds
const int CLUSTER_MF_PAUSE_MULT = 2;
const int CLUSTER_SLAVE_MIGRATION_DELAY = 5000; // 5 seconds

// Forward declarations
class ClusterNode;
class ClusterState;
class ClusterTransport;
class RedisCluster;

// Utility functions
namespace ClusterUtils {
    std::string generateNodeId() {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA_CTX sha1;
        SHA1_Init(&sha1);
        
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        std::random_device rd;
        auto random_value = rd();
        
        SHA1_Update(&sha1, &now, sizeof(now));
        SHA1_Update(&sha1, &random_value, sizeof(random_value));
        SHA1_Final(hash, &sha1);
        
        std::stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        
        return ss.str();
    }
    
    uint16_t crc16(const char *buf, int len) {
        static const uint16_t crc16tab[256] = {
            0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
            0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
            0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
            0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
            0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
            0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
            0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
            0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
            0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
            0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
            0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
            0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
            0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
            0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
            0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
            0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
            0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
            0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
            0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
            0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
            0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
            0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
            0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
            0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
            0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
            0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
            0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
            0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
            0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
            0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
            0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
            0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
        };
        
        uint16_t crc = 0;
        for (int i = 0; i < len; i++) {
            crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ buf[i]) & 0x00FF];
        }
        return crc;
    }
    
    int keyHashSlot(const char *key, size_t keylen) {
        size_t s, e; /* start-end indexes of { and } */
        
        for (s = 0; s < keylen; s++)
            if (key[s] == '{') break;
        
        /* No '{' ? Hash the whole key. */
        if (s == keylen) return crc16(key, keylen) & (CLUSTER_SLOTS-1);
        
        /* '{' found? Check if we have the corresponding '}'. */
        for (e = s+1; e < keylen; e++)
            if (key[e] == '}') break;
        
        /* No '}' or nothing between {} ? Hash the whole key. */
        if (e == keylen || e == s+1) return crc16(key, keylen) & (CLUSTER_SLOTS-1);
        
        /* If we are here there is both a { and a } on its right. Hash
         * what is in the middle between { and }. */
        return crc16(key+s+1, e-s-1) & (CLUSTER_SLOTS-1);
    }
}

// Cluster Node class
class ClusterNode {
public:
    std::string id;
    std::string ip;
    int port;
    bool is_master;
    bool is_myself;
    ClusterNode* master;
    std::vector<ClusterNode*> slaves;
    std::set<int> slots;
    std::set<int> migrating_slots_to;
    std::set<int> importing_slots_from;
    std::chrono::steady_clock::time_point ping_sent;
    std::chrono::steady_clock::time_point pong_received;
    int flags;
    
    ClusterNode(std::string node_id, std::string node_ip, int node_port, bool master, bool myself = false)
        : id(node_id), ip(node_ip), port(node_port), is_master(master), is_myself(myself),
          master(nullptr), flags(0) {
        ping_sent = std::chrono::steady_clock::now();
        pong_received = std::chrono::steady_clock::now();
    }
    
    bool hasFlag(int flag) const { return (flags & flag) != 0; }
    void setFlag(int flag) { flags |= flag; }
    void clearFlag(int flag) { flags &= ~flag; }
    
    bool isFailed() const { return hasFlag(CLUSTER_NODE_FAIL); }
    bool isHandshake() const { return hasFlag(CLUSTER_NODE_HANDSHAKE); }
    bool isNoAddr() const { return hasFlag(CLUSTER_NODE_NOADDR); }
    bool isPfail() const { return hasFlag(CLUSTER_NODE_PFAIL); }
    
    void markAsFailing() { setFlag(CLUSTER_NODE_PFAIL); }
    void clearFailing() { clearFlag(CLUSTER_NODE_PFAIL); }
    
    void addSlot(int slot) { slots.insert(slot); }
    void removeSlot(int slot) { slots.erase(slot); }
    bool hasSlot(int slot) const { return slots.find(slot) != slots.end(); }
    
    void addSlave(ClusterNode* slave) {
        slaves.push_back(slave);
        slave->master = this;
    }
    
    bool operator==(const ClusterNode& other) const {
        return id == other.id;
    }
    
    bool operator!=(const ClusterNode& other) const {
        return !(*this == other);
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << id << " " << ip << ":" << port << " " << (is_master ? "master" : "slave");
        if (master) ss << " master:" << master->id;
        return ss.str();
    }
    
    // Node flags
    static const int CLUSTER_NODE_MASTER = 1;
    static const int CLUSTER_NODE_SLAVE = 2;
    static const int CLUSTER_NODE_PFAIL = 4;
    static const int CLUSTER_NODE_FAIL = 8;
    static const int CLUSTER_NODE_MYSELF = 16;
    static const int CLUSTER_NODE_HANDSHAKE = 32;
    static const int CLUSTER_NODE_NOADDR = 64;
    static const int CLUSTER_NODE_MIGRATE_TO = 128;
};

// Cluster State class
class ClusterState {
private:
    std::mutex state_mutex;
    int current_epoch;
    int last_vote_epoch;
    ClusterNode* myself;
    std::map<std::string, std::shared_ptr<ClusterNode>> nodes;
    std::vector<ClusterNode*> masters;
    std::array<ClusterNode*, CLUSTER_SLOTS> slots;
    std::atomic<bool> failover_auth;
    std::atomic<bool> failover_auth_sent;
    std::atomic<int> failover_auth_epoch;
    std::atomic<bool> mf_end;
    std::chrono::steady_clock::time_point mf_slave_offset;
    
public:
    ClusterState() : current_epoch(0), last_vote_epoch(0), myself(nullptr), 
                    failover_auth(false), failover_auth_sent(false), 
                    failover_auth_epoch(0), mf_end(false) {
        slots.fill(nullptr);
    }
    
    void setMyself(ClusterNode* node) { 
        std::lock_guard<std::mutex> lock(state_mutex);
        myself = node; 
    }
    
    ClusterNode* getMyself()  { 
        std::lock_guard<std::mutex> lock(state_mutex);
        return myself; 
    }
    
    void addNode(std::shared_ptr<ClusterNode> node) {
        std::lock_guard<std::mutex> lock(state_mutex);
        nodes[node->id] = node;
        
        if (node->is_master) {
            masters.push_back(node.get());
        }
    }
    
    void removeNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(state_mutex);
        auto it = nodes.find(node_id);
        if (it != nodes.end()) {
            if (it->second->is_master) {
                masters.erase(std::remove(masters.begin(), masters.end(), it->second.get()), masters.end());
            }
            nodes.erase(it);
        }
    }
    
    ClusterNode* getNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(state_mutex);
        auto it = nodes.find(node_id);
        return it != nodes.end() ? it->second.get() : nullptr;
    }
    
    std::vector<ClusterNode*> getAllNodes() {
        std::lock_guard<std::mutex> lock(state_mutex);
        std::vector<ClusterNode*> result;
        for (auto& pair : nodes) {
            result.push_back(pair.second.get());
        }
        return result;
    }
    
    std::vector<ClusterNode*> getMasters() {
        std::lock_guard<std::mutex> lock(state_mutex);
        return masters;
    }
    
    int getMasterCount() {
        std::lock_guard<std::mutex> lock(state_mutex);
        return masters.size();
    }
    
    void assignSlot(int slot, ClusterNode* node) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS) {
            if (slots[slot]) {
                slots[slot]->removeSlot(slot);
            }
            slots[slot] = node;
            node->addSlot(slot);
        }
    }
    
    ClusterNode* getNodeBySlot(int slot) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS) {
            return slots[slot];
        }
        return nullptr;
    }
    
    void setMigrating(int slot, ClusterNode* target) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS && slots[slot]) {
            slots[slot]->migrating_slots_to.insert(slot);
            target->importing_slots_from.insert(slot);
        }
    }
    
    void clearMigrating(int slot) {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS && slots[slot]) {
            slots[slot]->migrating_slots_to.erase(slot);
            for (auto& node : nodes) {
                node.second->importing_slots_from.erase(slot);
            }
        }
    }
    
    void failover(const std::string& failed_node_id) {
        std::lock_guard<std::mutex> lock(state_mutex);
        current_epoch++;
        
        auto it = nodes.find(failed_node_id);
        if (it == nodes.end()) return;
        
        auto& failed_node = it->second;
        if (!failed_node->is_master) return;
        
        // Select the best slave to promote
        ClusterNode* new_master = selectPromotableSlave(failed_node.get());
        if (!new_master) return;
        
        // Perform the failover
        promoteSlaveToMaster(new_master, failed_node.get());
        
        // Update cluster state
        updateAfterFailover(failed_node.get(), new_master);
    }
    
    void startFailover() {
        failover_auth = false;
        failover_auth_sent = false;
        failover_auth_epoch = current_epoch;
    }
    
    bool getFailoverAuth() const { return failover_auth; }
    void setFailoverAuth(bool value) { failover_auth = value; }
    
    bool getFailoverAuthSent() const { return failover_auth_sent; }
    void setFailoverAuthSent(bool value) { failover_auth_sent = value; }
    
    int getFailoverAuthEpoch() const { return failover_auth_epoch; }
    void setFailoverAuthEpoch(int epoch) { failover_auth_epoch = epoch; }
    
    int getCurrentEpoch() const { return current_epoch; }
    void setCurrentEpoch(int epoch) { current_epoch = epoch; }
    
    int getLastVoteEpoch() const { return last_vote_epoch; }
    void setLastVoteEpoch(int epoch) { last_vote_epoch = epoch; }
    
    bool isMigrating() const { return !mf_end; }
    void setMigrating(bool value) { mf_end = !value; }
    
    void setMfSlaveOffset() { mf_slave_offset = std::chrono::steady_clock::now(); }
    auto getMfSlaveOffset() const { return mf_slave_offset; }
    
private:
    ClusterNode* selectPromotableSlave(ClusterNode* failed_master) {
        // Select the slave with the best replication offset
        ClusterNode* best_slave = nullptr;
        std::chrono::steady_clock::time_point best_offset;
        
        for (auto slave : failed_master->slaves) {
            if (!slave) continue;
            
            if (!best_slave || slave->pong_received > best_offset) {
                best_slave = slave;
                best_offset = slave->pong_received;
            }
        }
        
        return best_slave;
    }
    
    void promoteSlaveToMaster(ClusterNode* slave, ClusterNode* old_master) {
        slave->is_master = true;
        slave->master = nullptr;
        slave->slots = old_master->slots;
        
        // Update slot mapping
        for (int slot : slave->slots) {
            slots[slot] = slave;
        }
        
        // Redirect slaves to new master
        for (auto old_slave : old_master->slaves) {
            if (old_slave && old_slave != slave) {
                old_slave->master = slave;
                slave->slaves.push_back(old_slave);
            }
        }
    }
    
    void updateAfterFailover(ClusterNode* old_master, ClusterNode* new_master) {
        // Remove the old master
        nodes.erase(old_master->id);
        
        // Update masters list
        auto it = std::find(masters.begin(), masters.end(), old_master);
        if (it != masters.end()) {
            *it = new_master;
        }
    }
};

// Cluster Transport class
class ClusterTransport {
private:
    std::mutex transport_mutex;
    std::map<std::string, int> node_sockets;
    std::map<int, std::string> socket_nodes;
    std::thread receiver_thread;
    std::atomic<bool> running;
    std::queue<std::pair<std::string, std::string>> message_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    
public:
    ClusterTransport() : running(false) {}
    
    ~ClusterTransport() {
        stop();
    }
    
    void start() {
        running = true;
        receiver_thread = std::thread(&ClusterTransport::receiveMessages, this);
    }
    
    void stop() {
        running = false;
        queue_cv.notify_all();
        if (receiver_thread.joinable()) {
            receiver_thread.join();
        }
        
        std::lock_guard<std::mutex> lock(transport_mutex);
        for (auto& pair : node_sockets) {
            close(pair.second);
        }
        node_sockets.clear();
        socket_nodes.clear();
    }
    
    bool connectToNode(const std::string& node_id, const std::string& ip, int port) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket creation failed");
            return false;
        }
        
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
            perror("invalid address");
            close(sockfd);
            return false;
        }
        
        if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("connection failed");
            close(sockfd);
            return false;
        }
        
        // Set non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
        std::lock_guard<std::mutex> lock(transport_mutex);
        node_sockets[node_id] = sockfd;
        socket_nodes[sockfd] = node_id;
        
        return true;
    }
    
    void disconnectNode(const std::string& node_id) {
        std::lock_guard<std::mutex> lock(transport_mutex);
        auto it = node_sockets.find(node_id);
        if (it != node_sockets.end()) {
            close(it->second);
            socket_nodes.erase(it->second);
            node_sockets.erase(it);
        }
    }
    
    bool send(const std::string& node_id, const std::string& message) {
        std::lock_guard<std::mutex> lock(transport_mutex);
        auto it = node_sockets.find(node_id);
        if (it == node_sockets.end()) return false;
        
        int sockfd = it->second;
        std::string msg = message + "\n";
        ssize_t bytes_sent = write(sockfd, msg.c_str(), msg.size());
        return bytes_sent > 0;
    }
    
    void broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(transport_mutex);
        for (auto& pair : node_sockets) {
            std::string msg = message + "\n";
            write(pair.second, msg.c_str(), msg.size());
        }
    }
    
    std::pair<std::string, std::string> receive() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [this](){ return !message_queue.empty() || !running; });
        
        if (!running) return {"", ""};
        
        auto msg = message_queue.front();
        message_queue.pop();
        return msg;
    }
    
private:
    void receiveMessages() {
        while (running) {
            std::vector<pollfd> fds;
            {
                std::lock_guard<std::mutex> lock(transport_mutex);
                for (auto& pair : node_sockets) {
                    pollfd pfd;
                    pfd.fd = pair.second;
                    pfd.events = POLLIN;
                    fds.push_back(pfd);
                }
            }
            
            if (fds.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            int ret = poll(fds.data(), fds.size(), 100);
            if (ret <= 0) continue;
            
            for (auto& pfd : fds) {
                if (pfd.revents & POLLIN) {
                    char buffer[1024];
                    ssize_t bytes_read = read(pfd.fd, buffer, sizeof(buffer)-1);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        std::string node_id;
                        {
                            std::lock_guard<std::mutex> lock(transport_mutex);
                            auto it = socket_nodes.find(pfd.fd);
                            if (it != socket_nodes.end()) {
                                node_id = it->second;
                            }
                        }
                        
                        if (!node_id.empty()) {
                            std::lock_guard<std::mutex> lock(queue_mutex);
                            message_queue.emplace(node_id, std::string(buffer, bytes_read));
                            queue_cv.notify_one();
                        }
                    }
                }
            }
        }
    }
};

// Redis Cluster class
class RedisCluster {
private:
    ClusterState state;
    ClusterTransport transport;
    std::string self_id;
    std::thread cluster_cron_thread;
    std::atomic<bool> running;
    
public:
    RedisCluster(const std::string& ip, int port) : running(false) {
        self_id = ClusterUtils::generateNodeId();
        auto self = std::make_shared<ClusterNode>(self_id, ip, port, true, true);
        state.addNode(self);
        state.setMyself(self.get());
        
        transport.start();
    }
    
    ~RedisCluster() {
        stop();
    }
    
    void start() {
        running = true;
        cluster_cron_thread = std::thread(&RedisCluster::clusterCron, this);
    }
    
    void stop() {
        running = false;
        if (cluster_cron_thread.joinable()) {
            cluster_cron_thread.join();
        }
        transport.stop();
    }
    
    void addNode(const std::string& ip, int port, bool is_master) {
        std::string node_id = ClusterUtils::generateNodeId();
        auto node = std::make_shared<ClusterNode>(node_id, ip, port, is_master);
        state.addNode(node);
        
        if (transport.connectToNode(node_id, ip, port)) {
            // Send MEET message to the new node
            transport.send(node_id, "CLUSTER MEET " + ip + " " + std::to_string(port));
            
            // If we're adding a slave, find a master for it
            if (!is_master) {
                auto masters = state.getMasters();
                if (!masters.empty()) {
                    ClusterNode* master = masters[0];
                    transport.send(node_id, "CLUSTER REPLICATE " + master->id);
                }
            }
        }
    }
    
    void removeNode(const std::string& node_id) {
        state.removeNode(node_id);
        transport.disconnectNode(node_id);
    }
    
    void initSlots() {
        auto masters = state.getMasters();
        int master_count = masters.size();
        if (master_count == 0) return;
        
        int slots_per_node = CLUSTER_SLOTS / master_count;
        int remaining_slots = CLUSTER_SLOTS % master_count;
        int slot = 0;
        
        for (size_t i = 0; i < masters.size(); ++i) {
            int end_slot = slot + slots_per_node;
            if (i < remaining_slots) end_slot++;
            
            for (; slot < end_slot; ++slot) {
                state.assignSlot(slot, masters[i]);
            }
        }
    }
    
    void migrateSlot(int slot, const std::string& target_node_id) {
        ClusterNode* source_node = state.getNodeBySlot(slot);
        ClusterNode* target_node = state.getNode(target_node_id);
        
        if (!source_node || !target_node || source_node == target_node) return;
        
        // 1. Set migrating/importing state
        state.setMigrating(slot, target_node);
        
        // 2. Send MIGRATE command to source node
        transport.send(source_node->id, "CLUSTER MIGRATE " + target_node->ip + " " + 
                       std::to_string(target_node->port) + " " + std::to_string(slot));
        
        // 3. Wait for migration to complete (in a real implementation, this would be async)
        // ...
        
        // 4. Update slot ownership
        state.assignSlot(slot, target_node);
        
        // 5. Clear migration state
        state.clearMigrating(slot);
        
        // 6. Broadcast the new configuration
        broadcastSlotUpdate(slot, target_node);
    }
    
    void failover() {
        ClusterNode* myself = state.getMyself();
        if (!myself || myself->is_master) return;
        
        ClusterNode* master = myself->master;
        if (!master) return;
        
        // Check if master is really failing
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - master->pong_received).count();
        
        if (elapsed > CLUSTER_NODE_TIMEOUT) {
            // Start failover process
            state.startFailover();
            
            // Request votes from other masters
            for (auto node : state.getMasters()) {
                if (node != master) {
                    transport.send(node->id, "CLUSTER FAILOVER_AUTH_REQUEST " + 
                                  std::to_string(state.getCurrentEpoch()));
                }
            }
            
            // Wait for votes (simplified)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            if (state.getFailoverAuth()) {
                // Promote myself to master
                myself->is_master = true;
                myself->master = nullptr;
                
                // Take over master's slots
                for (int slot : master->slots) {
                    state.assignSlot(slot, myself);
                }
                
                // Broadcast the new configuration
                broadcastConfigUpdate();
            }
        }
    }
    
    std::string processCommand(const std::string& key, const std::string& command) {
        int slot = ClusterUtils::keyHashSlot(key.c_str(), key.size());
        ClusterNode* node = state.getNodeBySlot(slot);
        
        if (!node) {
            return "-CLUSTERDOWN The cluster is down\r\n";
        }
        
        ClusterNode* myself = state.getMyself();
        if (node == myself) {
            // Execute command locally
            return executeLocalCommand(command);
        } else {
            // Redirect to the right node
            return redirectCommand(node, slot, command);
        }
    }
    
private:
    void clusterCron() {
        while (running) {
            // 1. Check for failed nodes
            checkForFailedNodes();
            
            // 2. Perform automatic failover if needed
            autoFailover();
            
            // 3. Rebalance slots if needed
            rebalanceSlots();
            
            // 4. Update cluster state
            updateClusterState();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void checkForFailedNodes() {
        auto now = std::chrono::steady_clock::now();
        auto nodes = state.getAllNodes();
        
        for (auto node : nodes) {
            if (node == state.getMyself()) continue;
            
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - node->pong_received).count();
            if (elapsed > CLUSTER_NODE_TIMEOUT) {
                if (!node->isPfail()) {
                    node->markAsFailing();
                    broadcastNodeFailure(node);
                }
            } else if (node->isPfail()) {
                node->clearFailing();
                broadcastNodeRecovery(node);
            }
        }
    }
    
    void autoFailover() {
        ClusterNode* myself = state.getMyself();
        if (!myself || myself->is_master) return;
        
        ClusterNode* master = myself->master;
        if (!master || !master->isFailed()) return;
        
        // Check if we're the best candidate for failover
        if (isBestSlaveForFailover(master)) {
            failover();
        }
    }
    
    bool isBestSlaveForFailover(ClusterNode* master) {
        // Simplified version - in reality, we'd compare replication offsets
        auto slaves = master->slaves;
        return !slaves.empty() && slaves[0] == state.getMyself();
    }
    
    void rebalanceSlots() {
        // Simplified slot rebalancing logic
        auto masters = state.getMasters();
        if (masters.size() < 2) return;
        
        // Calculate average slots per node
        size_t total_slots = 0;
        for (auto master : masters) {
            total_slots += master->slots.size();
        }
        size_t target_slots = total_slots / masters.size();
        
        // Find nodes with too many or too few slots
        std::vector<ClusterNode*> sources, targets;
        for (auto master : masters) {
            if (master->slots.size() > target_slots) {
                sources.push_back(master);
            } else if (master->slots.size() < target_slots) {
                targets.push_back(master);
            }
        }
        
        // Perform slot migration
        for (size_t i = 0; i < sources.size() && i < targets.size(); ++i) {
            auto source = sources[i];
            auto target = targets[i];
            
            if (!source->slots.empty()) {
                int slot = *source->slots.begin();
                migrateSlot(slot, target->id);
            }
        }
    }
    
    void updateClusterState() {
        // In a real implementation, this would update the cluster state
        // based on gossip messages and other events
    }
    
    void broadcastNodeFailure(ClusterNode* node) {
        transport.broadcast("CLUSTER NODE_FAIL " + node->id);
    }
    
    void broadcastNodeRecovery(ClusterNode* node) {
        transport.broadcast("CLUSTER NODE_RECOVERED " + node->id);
    }
    
    void broadcastSlotUpdate(int slot, ClusterNode* node) {
        transport.broadcast("CLUSTER SLOT_UPDATE " + std::to_string(slot) + " " + node->id);
    }
    
    void broadcastConfigUpdate() {
        transport.broadcast("CLUSTER CONFIG_UPDATE");
    }
    
    std::string executeLocalCommand(const std::string& command) {
        // Simplified command execution
        if (command.substr(0, 3) == "GET") {
            return "+OK\r\n"; // In a real implementation, this would return the actual value
        } else if (command.substr(0, 3) == "SET") {
            return "+OK\r\n";
        } else {
            return "-ERR unknown command\r\n";
        }
    }
    
    std::string redirectCommand(ClusterNode* node, int slot, const std::string& command) {
        std::stringstream ss;
        ss << "-MOVED " << slot << " " << node->ip << ":" << node->port << "\r\n";
        return ss.str();
    }
};

int main() {
    // Example usage
    RedisCluster cluster("127.0.0.1", 7000);
    cluster.start();
    
    // Add nodes to form a 3-master cluster
    cluster.addNode("127.0.0.1", 7001, true);
    cluster.addNode("127.0.0.1", 7002, true);
    
    // Initialize slot distribution
    cluster.initSlots();
    
    // Simulate some operations
    std::cout << cluster.processCommand("mykey", "SET mykey myvalue") << std::endl;
    
    std::cout << cluster.processCommand("mykey", "GET mykey") << std::endl;
    
    // Wait for a while
    while(1){};
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    cluster.stop();
    return 0;
}

