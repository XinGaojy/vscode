#if 0
#include <vector>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
using namespace std;

template<typename KeyType, typename ValueType>
class HashMap {
private:
    struct Bucket {
        std::list<std::pair<KeyType, ValueType>> ls;
        std::mutex mtx;
    };
    std::vector<Bucket> table;

    size_t hashFunction(const KeyType &key) {
        return std::hash<KeyType>()(key) % table.size();
    }

public:
    HashMap(size_t size = 1001) : table(size) {}

    void insert(const KeyType& key, const ValueType& value) {
        size_t index = hashFunction(key);
        auto &bucket = table[index];
        std::unique_lock<std::mutex> lk(bucket.mtx);
        for (auto &kv : bucket.ls) {
            if (kv.first == key) {
                kv.second = value;
                return; // 键存在
            }
        }
        bucket.ls.emplace_back(key, value); // 键不存在
    }

    bool get(const KeyType& key, ValueType& value) {
        size_t index = hashFunction(key);
        auto &bucket = table[index];
        std::unique_lock<std::mutex> lk(bucket.mtx);
        for (auto &kv : bucket.ls) {
            if (kv.first == key) {
                value = kv.second;
                return true; // 键存在
            }
        }
        return false; // 键不存在
    }

    bool erase(const KeyType& key) {
        size_t index = hashFunction(key);
        auto &bucket = table[index];
        std::unique_lock<std::mutex> lk(bucket.mtx);
        for (auto it = bucket.ls.begin(); it != bucket.ls.end(); ++it) {
            if (it->first == key) {
                bucket.ls.erase(it);
                return true; 
            }
        }
        return false; // 键不存在
    }
};

int main() {
    HashMap<int, int> mp;
    for (int i = 0; i < 5; i++) {
        mp.insert(i, i * 10);
    }
    for (int i = 0; i < 5; i++) {
        int value = -1;
        if (mp.get(i, value)) {
            cout << i << " key find, value = " << value << endl;
        } else {
            cout << i << " key not find" << endl;
        }
    }
    for (int i = 0; i < 2; i++) {
        mp.erase(i);
    }
    cout << "==================" << endl;
    for (int i = 0; i < 5; i++) {
        int value = -1;
        if (mp.get(i, value)) {
            cout << i << " key find, value = " << value << endl;
        } else {
            cout << i << " key not find" << endl;
        }
    }
}

#endif




