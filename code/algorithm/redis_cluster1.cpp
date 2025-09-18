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
#include <signal.h>

using namespace std;

// Constants
const int CLUSTER_SLOTS = 16384;
const int CLUSTER_PORT_BASE = 7000;
const int CLUSTER_NODE_TIMEOUT = 15000; // 15 seconds
const int MAX_CLIENT_BUFFER = 1024;

// Forward declarations
class ClusterNode;
class ClusterState;
class ClusterTransport;
class RedisCluster;

// Utility functions
namespace ClusterUtils {
    string generateNodeId() {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA_CTX sha1;
        SHA1_Init(&sha1);
        
        auto now = chrono::system_clock::now().time_since_epoch().count();
        random_device rd;
        auto random_value = rd();
        
        SHA1_Update(&sha1, &now, sizeof(now));
        SHA1_Update(&sha1, &random_value, sizeof(random_value));
        SHA1_Final(hash, &sha1);
        
        stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            ss << hex << setw(2) << setfill('0') << (int)hash[i];
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
        size_t s, e; // start-end indexes of { and }
        
        for (s = 0; s < keylen; s++)
            if (key[s] == '{') break;
        
        if (s == keylen) return crc16(key, keylen) & (CLUSTER_SLOTS-1);
        
        for (e = s+1; e < keylen; e++)
            if (key[e] == '}') break;
        
        if (e == keylen || e == s+1) return crc16(key, keylen) & (CLUSTER_SLOTS-1);
        
        return crc16(key+s+1, e-s-1) & (CLUSTER_SLOTS-1);
    }
}

// Redis protocol utilities
namespace RedisProtocol {
    string encodeSimpleString(const string& str) {
        return "+" + str + "\r\n";
    }
    
    string encodeError(const string& err) {
        return "-" + err + "\r\n";
    }
    
    string encodeInteger(int64_t num) {
        return ":" + to_string(num) + "\r\n";
    }
    
    string encodeBulkString(const string& str) {
        if (str.empty()) return "$-1\r\n";
        return "$" + to_string(str.size()) + "\r\n" + str + "\r\n";
    }
    
    string encodeArray(const vector<string>& elements) {
        string result = "*" + to_string(elements.size()) + "\r\n";
        for (const auto& elem : elements) {
            result += elem;
        }
        return result;
    }
    
    vector<string> parseCommand(const string& command) {
        vector<string> args;
        size_t pos = 0;
        
        if (command.empty()) return args;
        
        if (command[0] != '*') {
            // Simple protocol (inline command)
            istringstream iss(command);
            string arg;
            while (iss >> arg) {
                args.push_back(arg);
            }
            return args;
        }
        
        // Multi-bulk protocol
        pos = command.find('\n');
        if (pos == string::npos) return args;
        
        int arg_count = stoi(command.substr(1, pos - 1));
        string remaining = command.substr(pos + 1);
        
        for (int i = 0; i < arg_count; ++i) {
            if (remaining.empty() || remaining[0] != '$') return {};
            
            pos = remaining.find('\n');
            if (pos == string::npos) return {};
            
            int arg_len = stoi(remaining.substr(1, pos - 1));
            remaining = remaining.substr(pos + 1);
            
            if (remaining.length() < arg_len + 2) return {};
            
            string arg = remaining.substr(0, arg_len);
            args.push_back(arg);
            remaining = remaining.substr(arg_len + 2);
        }
        
        return args;
    }
}

// Cluster Node class
class ClusterNode {
public:
    string id;
    string ip;
    int port;
    bool is_master;
    bool is_myself;
    ClusterNode* master;
    vector<ClusterNode*> slaves;
    set<int> slots;
    set<int> migrating_slots_to;
    set<int> importing_slots_from;
    chrono::steady_clock::time_point ping_sent;
    chrono::steady_clock::time_point pong_received;
    int flags;
    
    // Node flags
    static const int NODE_MASTER = 1;
    static const int NODE_SLAVE = 2;
    static const int NODE_PFAIL = 4;
    static const int NODE_FAIL = 8;
    static const int NODE_MYSELF = 16;
    static const int NODE_HANDSHAKE = 32;
    static const int NODE_NOADDR = 64;
    static const int NODE_MIGRATE_TO = 128;
    
    ClusterNode(string node_id, string node_ip, int node_port, bool master, bool myself = false)
        : id(node_id), ip(node_ip), port(node_port), is_master(master), is_myself(myself),
          master(nullptr), flags(0) {
        ping_sent = chrono::steady_clock::now();
        pong_received = chrono::steady_clock::now();
        if (is_master) flags |= NODE_MASTER;
        if (!is_master) flags |= NODE_SLAVE;
        if (is_myself) flags |= NODE_MYSELF;
    }
    
    bool hasFlag(int flag) const { return (flags & flag) != 0; }
    void setFlag(int flag) { flags |= flag; }
    void clearFlag(int flag) { flags &= ~flag; }
    
    bool isFailed() const { return hasFlag(NODE_FAIL); }
    bool isPfail() const { return hasFlag(NODE_PFAIL); }
    
    void markAsFailing() { setFlag(NODE_PFAIL); }
    void clearFailing() { clearFlag(NODE_PFAIL); }
    
    void addSlot(int slot) { slots.insert(slot); }
    void removeSlot(int slot) { slots.erase(slot); }
    bool hasSlot(int slot) const { return slots.find(slot) != slots.end(); }
    
    void addSlave(ClusterNode* slave) {
        slaves.push_back(slave);
        slave->master = this;
        slave->is_master = false;
        slave->clearFlag(NODE_MASTER);
        slave->setFlag(NODE_SLAVE);
    }
    
    bool operator==(const ClusterNode& other) const {
        return id == other.id;
    }
    
    bool operator!=(const ClusterNode& other) const {
        return !(*this == other);
    }
    
    string toString() const {
        stringstream ss;
        ss << id << " " << ip << ":" << port << " " << (is_master ? "master" : "slave");
        if (master) ss << " master:" << master->id;
        return ss.str();
    }
};

// Cluster State class
class ClusterState {
private:
    mutex state_mutex;
    int current_epoch;
    int last_vote_epoch;
    ClusterNode* myself;
    map<string, shared_ptr<ClusterNode>> nodes;
    vector<ClusterNode*> masters;
    array<ClusterNode*, CLUSTER_SLOTS> slots;
    atomic<bool> failover_auth;
    atomic<bool> failover_auth_sent;
    atomic<int> failover_auth_epoch;
    atomic<bool> mf_end;
    chrono::steady_clock::time_point mf_slave_offset;
    
public:
    ClusterState() : current_epoch(0), last_vote_epoch(0), myself(nullptr), 
                    failover_auth(false), failover_auth_sent(false), 
                    failover_auth_epoch(0), mf_end(false) {
        slots.fill(nullptr);
    }
    
    void setMyself(ClusterNode* node) { 
        lock_guard<mutex> lock(state_mutex);
        myself = node; 
    }
    
    ClusterNode* getMyself() { 
        lock_guard<mutex> lock(state_mutex);
        return myself; 
    }
    
    void addNode(shared_ptr<ClusterNode> node) {
        lock_guard<mutex> lock(state_mutex);
        nodes[node->id] = node;
        
        if (node->is_master) {
            masters.push_back(node.get());
        }
    }
    
    void removeNode(const string& node_id) {
        lock_guard<mutex> lock(state_mutex);
        auto it = nodes.find(node_id);
        if (it != nodes.end()) {
            if (it->second->is_master) {
                masters.erase(remove(masters.begin(), masters.end(), it->second.get()), masters.end());
            }
            nodes.erase(it);
        }
    }
    
    ClusterNode* getNode(const string& node_id) {
        lock_guard<mutex> lock(state_mutex);
        auto it = nodes.find(node_id);
        return it != nodes.end() ? it->second.get() : nullptr;
    }
    
    vector<ClusterNode*> getAllNodes() {
        lock_guard<mutex> lock(state_mutex);
        vector<ClusterNode*> result;
        for (auto& pair : nodes) {
            result.push_back(pair.second.get());
        }
        return result;
    }
    
    vector<ClusterNode*> getMasters() {
        lock_guard<mutex> lock(state_mutex);
        return masters;
    }
    
    int getMasterCount() {
        lock_guard<mutex> lock(state_mutex);
        return masters.size();
    }
    
    void assignSlot(int slot, ClusterNode* node) {
        lock_guard<mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS) {
            if (slots[slot]) {
                slots[slot]->removeSlot(slot);
            }
            slots[slot] = node;
            node->addSlot(slot);
        }
    }
    
    ClusterNode* getNodeBySlot(int slot) {
        lock_guard<mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS) {
            return slots[slot];
        }
        return nullptr;
    }
    
    void setMigrating(int slot, ClusterNode* target) {
        lock_guard<mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS && slots[slot]) {
            slots[slot]->migrating_slots_to.insert(slot);
            target->importing_slots_from.insert(slot);
        }
    }
    
    void clearMigrating(int slot) {
        lock_guard<mutex> lock(state_mutex);
        if (slot >= 0 && slot < CLUSTER_SLOTS && slots[slot]) {
            slots[slot]->migrating_slots_to.erase(slot);
            for (auto& node : nodes) {
                node.second->importing_slots_from.erase(slot);
            }
        }
    }
    
    void failover(const string& failed_node_id) {
        lock_guard<mutex> lock(state_mutex);
        current_epoch++;
        
        auto it = nodes.find(failed_node_id);
        if (it == nodes.end()) return;
        
        auto& failed_node = it->second;
        if (!failed_node->is_master) return;
        
        ClusterNode* new_master = selectPromotableSlave(failed_node.get());
        if (!new_master) return;
        
        promoteSlaveToMaster(new_master, failed_node.get());
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
    
    void setMfSlaveOffset() { mf_slave_offset = chrono::steady_clock::now(); }
    auto getMfSlaveOffset() const { return mf_slave_offset; }
    
private:
    ClusterNode* selectPromotableSlave(ClusterNode* failed_master) {
        ClusterNode* best_slave = nullptr;
        chrono::steady_clock::time_point best_offset;
        
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
        slave->clearFlag(ClusterNode::NODE_SLAVE);
        slave->setFlag(ClusterNode::NODE_MASTER);
        
        for (int slot : slave->slots) {
            slots[slot] = slave;
        }
        
        for (auto old_slave : old_master->slaves) {
            if (old_slave && old_slave != slave) {
                old_slave->master = slave;
                slave->slaves.push_back(old_slave);
            }
        }
    }
    
    void updateAfterFailover(ClusterNode* old_master, ClusterNode* new_master) {
        nodes.erase(old_master->id);
        
        auto it = find(masters.begin(), masters.end(), old_master);
        if (it != masters.end()) {
            *it = new_master;
        }
    }
};

// Cluster Transport class
class ClusterTransport {
private:
    mutex transport_mutex;
    map<string, int> node_sockets;
    map<int, string> socket_nodes;
    thread receiver_thread;
    atomic<bool> running;
    queue<pair<string, string>> message_queue;
    mutex queue_mutex;
    condition_variable queue_cv;
    
public:
    ClusterTransport() : running(false) {}
    
    ~ClusterTransport() {
        stop();
    }
    
    void start() {
        running = true;
        receiver_thread = thread(&ClusterTransport::receiveMessages, this);
    }
    
    void stop() {
        running = false;
        queue_cv.notify_all();
        if (receiver_thread.joinable()) {
            receiver_thread.join();
        }
        
        lock_guard<mutex> lock(transport_mutex);
        for (auto& pair : node_sockets) {
            close(pair.second);
        }
        node_sockets.clear();
        socket_nodes.clear();
    }
    
    bool connectToNode(const string& node_id, const string& ip, int port) {
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
        
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
        lock_guard<mutex> lock(transport_mutex);
        node_sockets[node_id] = sockfd;
        socket_nodes[sockfd] = node_id;
        
        return true;
    }
    
    void disconnectNode(const string& node_id) {
        lock_guard<mutex> lock(transport_mutex);
        auto it = node_sockets.find(node_id);
        if (it != node_sockets.end()) {
            close(it->second);
            socket_nodes.erase(it->second);
            node_sockets.erase(it);
        }
    }
    
    bool send(const string& node_id, const string& message) {
        lock_guard<mutex> lock(transport_mutex);
        auto it = node_sockets.find(node_id);
        if (it == node_sockets.end()) return false;
        
        int sockfd = it->second;
        string msg = message + "\n";
        ssize_t bytes_sent = write(sockfd, msg.c_str(), msg.size());
        return bytes_sent > 0;
    }
    
    void broadcast(const string& message) {
        lock_guard<mutex> lock(transport_mutex);
        for (auto& pair : node_sockets) {
            string msg = message + "\n";
            write(pair.second, msg.c_str(), msg.size());
        }
    }
    
    pair<string, string> receive() {
        unique_lock<mutex> lock(queue_mutex);
        queue_cv.wait(lock, [this](){ return !message_queue.empty() || !running; });
        
        if (!running) return {"", ""};
        
        auto msg = message_queue.front();
        message_queue.pop();
        return msg;
    }
    
private:
    void receiveMessages() {
        while (running) {
            vector<pollfd> fds;
            {
                lock_guard<mutex> lock(transport_mutex);
                for (auto& pair : node_sockets) {
                    pollfd pfd;
                    pfd.fd = pair.second;
                    pfd.events = POLLIN;
                    fds.push_back(pfd);
                }
            }
            
            if (fds.empty()) {
                this_thread::sleep_for(chrono::milliseconds(100));
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
                        string node_id;
                        {
                            lock_guard<mutex> lock(transport_mutex);
                            auto it = socket_nodes.find(pfd.fd);
                            if (it != socket_nodes.end()) {
                                node_id = it->second;
                            }
                        }
                        
                        if (!node_id.empty()) {
                            lock_guard<mutex> lock(queue_mutex);
                            message_queue.emplace(node_id, string(buffer, bytes_read));
                            queue_cv.notify_one();
                        }
                    }
                }
            }
        }
    }
};



// Client service for handling external Redis clients
class ClusterClientService {
private:
    RedisCluster& cluster;
    int server_fd;
    atomic<bool> running;
    thread server_thread;
    mutex clients_mutex;
    vector<int> client_fds;
    
public:
    ClusterClientService(RedisCluster& cluster_ref, int port) 
        : cluster(cluster_ref), running(false) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw runtime_error("Failed to create server socket");
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw runtime_error("Failed to set socket options");
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw runtime_error("Failed to bind socket");
        }

        if (listen(server_fd, 128) < 0) {
            throw runtime_error("Failed to listen on socket");
        }
    }

    ~ClusterClientService() {
        stop();
    }

    void start() {
        running = true;
        server_thread = thread(&ClusterClientService::run, this);
    }

    void stop() {
        running = false;
        close(server_fd);
        if (server_thread.joinable()) {
            server_thread.join();
        }
        
        lock_guard<mutex> lock(clients_mutex);
        for (int fd : client_fds) {
            close(fd);
        }
        client_fds.clear();
    }

private:
    void run() {
        while (running) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_fd, &read_fds);
            
            int max_fd = server_fd;
            
            {
                lock_guard<mutex> lock(clients_mutex);
                for (int fd : client_fds) {
                    FD_SET(fd, &read_fds);
                    if (fd > max_fd) max_fd = fd;
                }
            }

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
            if (activity < 0 && errno != EINTR) {
                perror("select error");
                continue;
            }

            if (FD_ISSET(server_fd, &read_fds)) {
                handleNewConnection();
            }

            vector<int> to_remove;
            {
                lock_guard<mutex> lock(clients_mutex);
                for (int fd : client_fds) {
                    if (FD_ISSET(fd, &read_fds)) {
                        if (!handleClientRequest(fd)) {
                            to_remove.push_back(fd);
                        }
                    }
                }
            }

            for (int fd : to_remove) {
                close(fd);
                lock_guard<mutex> lock(clients_mutex);
                client_fds.erase(remove(client_fds.begin(), client_fds.end(), fd), client_fds.end());
            }
        }
    }

    void handleNewConnection() {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_fd < 0) {
            perror("accept failed");
            return;
        }

        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        lock_guard<mutex> lock(clients_mutex);
        client_fds.push_back(client_fd);
    }

    bool handleClientRequest(int client_fd) {
        char buffer[MAX_CLIENT_BUFFER];
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            return false; // Close connection
        }

        buffer[bytes_read] = '\0';
        string command(buffer, bytes_read);
        string response = cluster.processCommand(command);

        write(client_fd, response.c_str(), response.size());
        return true;
    }
};

// Main Redis Cluster class
class RedisCluster {
private:
    ClusterState state;
    ClusterTransport transport;
    ClusterClientService client_service;
    string self_id;
    thread cluster_cron_thread;
    atomic<bool> running;
    map<string, string> data_store;
    mutex data_mutex;
    
public:
    RedisCluster(const string& ip, int port) 
        : client_service(*this, port), running(false) {
        self_id = ClusterUtils::generateNodeId();
        auto self = make_shared<ClusterNode>(self_id, ip, port, true, true);
        state.addNode(self);
        state.setMyself(self.get());
        
        transport.start();
        client_service.start();
    }
    
    ~RedisCluster() {
        stop();
    }
    
    void start() {
        running = true;
        cluster_cron_thread = thread(&RedisCluster::clusterCron, this);
    }
    
    void stop() {
        running = false;
        if (cluster_cron_thread.joinable()) {
            cluster_cron_thread.join();
        }
        transport.stop();
        client_service.stop();
    }
    
    void addNode(const string& ip, int port, bool is_master) {
        string node_id = ClusterUtils::generateNodeId();
        auto node = make_shared<ClusterNode>(node_id, ip, port, is_master);
        state.addNode(node);
        
        if (transport.connectToNode(node_id, ip, port)) {
            transport.send(node_id, "CLUSTER MEET " + ip + " " + to_string(port));
            
            if (!is_master) {
                auto masters = state.getMasters();
                if (!masters.empty()) {
                    ClusterNode* master = masters[0];
                    transport.send(node_id, "CLUSTER REPLICATE " + master->id);
                }
            }
        }
    }
    
    void removeNode(const string& node_id) {
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
    
    void migrateSlot(int slot, const string& target_node_id) {
        ClusterNode* source_node = state.getNodeBySlot(slot);
        ClusterNode* target_node = state.getNode(target_node_id);
        
        if (!source_node || !target_node || source_node == target_node) return;
        
        state.setMigrating(slot, target_node);
        transport.send(source_node->id, "CLUSTER MIGRATE " + target_node->ip + " " + 
                       to_string(target_node->port) + " " + to_string(slot));
        
        this_thread::sleep_for(chrono::milliseconds(100));
        
        state.assignSlot(slot, target_node);
        state.clearMigrating(slot);
        broadcastSlotUpdate(slot, target_node);
    }
    
    void failover() {
        ClusterNode* myself = state.getMyself();
        if (!myself || myself->is_master) return;
        
        ClusterNode* master = myself->master;
        if (!master) return;
        
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - master->pong_received).count();
        
        if (elapsed > CLUSTER_NODE_TIMEOUT) {
            state.startFailover();
            
            for (auto node : state.getMasters()) {
                if (node != master) {
                    transport.send(node->id, "CLUSTER FAILOVER_AUTH_REQUEST " + 
                                  to_string(state.getCurrentEpoch()));
                }
            }
            
            this_thread::sleep_for(chrono::milliseconds(500));
            
            if (state.getFailoverAuth()) {
                myself->is_master = true;
                myself->master = nullptr;
                myself->clearFlag(ClusterNode::NODE_SLAVE);
                myself->setFlag(ClusterNode::NODE_MASTER);
                
                for (int slot : master->slots) {
                    state.assignSlot(slot, myself);
                }
                
                broadcastConfigUpdate();
            }
        }
    }
    
    string processCommand(const string& command) {
        auto args = RedisProtocol::parseCommand(command);
        if (args.empty()) {
            return RedisProtocol::encodeError("ERR empty command");
        }
        
        string cmd = args[0];
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        
        if (cmd == "PING") {
            return RedisProtocol::encodeSimpleString("PONG");
        }
        else if (cmd == "GET" && args.size() == 2) {
            return processGetCommand(args[1]);
        }
        else if (cmd == "SET" && args.size() == 3) {
            return processSetCommand(args[1], args[2]);
        }
        else if (cmd == "CLUSTER" && args.size() >= 2) {
            return processClusterCommand(args);
        }
        
        return RedisProtocol::encodeError("ERR unknown command");
    }
    
private:
    void clusterCron() {
        while (running) {
            checkForFailedNodes();
            autoFailover();
            rebalanceSlots();
            updateClusterState();
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    
    void checkForFailedNodes() {
        auto now = chrono::steady_clock::now();
        auto nodes = state.getAllNodes();
        
        for (auto node : nodes) {
            if (node == state.getMyself()) continue;
            
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - node->pong_received).count();
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
        
        if (isBestSlaveForFailover(master)) {
            failover();
        }
    }
    
    bool isBestSlaveForFailover(ClusterNode* master) {
        auto slaves = master->slaves;
        return !slaves.empty() && slaves[0] == state.getMyself();
    }
    
    void rebalanceSlots() {
        auto masters = state.getMasters();
        if (masters.size() < 2) return;
        
        size_t total_slots = 0;
        for (auto master : masters) {
            total_slots += master->slots.size();
        }
        size_t target_slots = total_slots / masters.size();
        
        vector<ClusterNode*> sources, targets;
        for (auto master : masters) {
            if (master->slots.size() > target_slots) {
                sources.push_back(master);
            } else if (master->slots.size() < target_slots) {
                targets.push_back(master);
            }
        }
        
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
    }
    
    string processGetCommand(const string& key) {
        int slot = ClusterUtils::keyHashSlot(key.c_str(), key.size());
        ClusterNode* node = state.getNodeBySlot(slot);
        
        if (!node) {
            return RedisProtocol::encodeError("CLUSTERDOWN The cluster is down");
        }
        
        if (node == state.getMyself()) {
            lock_guard<mutex> lock(data_mutex);
            auto it = data_store.find(key);
            if (it != data_store.end()) {
                return RedisProtocol::encodeBulkString(it->second);
            }
            return RedisProtocol::encodeBulkString("");
        } else {
            return RedisProtocol::encodeError("MOVED " + to_string(slot) + " " + 
                                            node->ip + ":" + to_string(node->port));
        }
    }
    
    string processSetCommand(const string& key, const string& value) {
        int slot = ClusterUtils::keyHashSlot(key.c_str(), key.size());
        ClusterNode* node = state.getNodeBySlot(slot);
        
        if (!node) {
            return RedisProtocol::encodeError("CLUSTERDOWN The cluster is down");
        }
        
        if (node == state.getMyself()) {
            lock_guard<mutex> lock(data_mutex);
            data_store[key] = value;
            return RedisProtocol::encodeSimpleString("OK");
        } else {
            return RedisProtocol::encodeError("MOVED " + to_string(slot) + " " + 
                                            node->ip + ":" + to_string(node->port));
        }
    }
    
    string processClusterCommand(const vector<string>& args) {
        string subcmd = args[1];
        transform(subcmd.begin(), subcmd.end(), subcmd.begin(), ::toupper);
        
        if (subcmd == "NODES") {
            string nodes_info;
            auto all_nodes = state.getAllNodes();
            for (auto node : all_nodes) {
                nodes_info += node->toString() + "\n";
            }
            return RedisProtocol::encodeBulkString(nodes_info);
        }
        else if (subcmd == "SLOTS") {
            vector<string> slot_info;
            for (int slot = 0; slot < CLUSTER_SLOTS; ++slot) {
                auto node = state.getNodeBySlot(slot);
                if (node) {
                    stringstream ss;
                    ss << slot << " " << node->ip << ":" << node->port << " " << node->id;
                    slot_info.push_back(RedisProtocol::encodeBulkString(ss.str()));
                }
            }
            return RedisProtocol::encodeArray(slot_info);
        }
        
        return RedisProtocol::encodeError("ERR unknown cluster subcommand");
    }
    
    void broadcastNodeFailure(ClusterNode* node) {
        transport.broadcast("CLUSTER NODE_FAIL " + node->id);
    }
    
    void broadcastNodeRecovery(ClusterNode* node) {
        transport.broadcast("CLUSTER NODE_RECOVERED " + node->id);
    }
    
    void broadcastSlotUpdate(int slot, ClusterNode* node) {
        transport.broadcast("CLUSTER SLOT_UPDATE " + to_string(slot) + " " + node->id);
    }
    
    void broadcastConfigUpdate() {
        transport.broadcast("CLUSTER CONFIG_UPDATE");
    }
};

// Signal handler for graceful shutdown
atomic<bool> shutdown_flag(false);
void signalHandler(int signum) {
    cout << "Interrupt signal (" << signum << ") received.\n";
    shutdown_flag = true;
}

int main() {
    // Register signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        // Create 3-node cluster
        vector<unique_ptr<RedisCluster>> nodes;
        for (int i = 0; i < 3; ++i) {
            nodes.emplace_back(make_unique<RedisCluster>("127.0.0.1", CLUSTER_PORT_BASE + i));
        }

        // Start all nodes
        for (auto& node : nodes) {
            node->start();
        }

        // Let nodes discover each other
        nodes[0]->addNode("127.0.0.1", CLUSTER_PORT_BASE + 1, true);
        nodes[0]->addNode("127.0.0.1", CLUSTER_PORT_BASE + 2, true);

        // Initialize slot distribution
        nodes[0]->initSlots();

        cout << "Redis cluster is running on ports " 
             << CLUSTER_PORT_BASE << "-" << CLUSTER_PORT_BASE+2 << endl;
        cout << "Connect using: redis-cli -p " << CLUSTER_PORT_BASE << endl;
        cout << "Press Ctrl+C to stop..." << endl;

        // Wait for shutdown signal
        while (!shutdown_flag) {
            this_thread::sleep_for(chrono::seconds(1));
        }

        // Stop all nodes
        for (auto& node : nodes) {
            node->stop();
        }

        cout << "Cluster stopped gracefully" << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}