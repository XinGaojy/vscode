#include <iostream>
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

// 使用函数回调替代接口，减少虚函数开销
class CallbackSubject {
public:
    using Callback = std::function<void(const std::string&)>;
    using CallbackHandle = size_t;
    
    CallbackHandle subscribe(Callback callback) {
        static CallbackHandle next_handle = 0;
        callbacks_[++next_handle] = callback;
        return next_handle;
    }
    
    void unsubscribe(CallbackHandle handle) {
        callbacks_.erase(handle);
    }
    
    void notify(const std::string& message) {
        for (const auto& [_, callback] : callbacks_) {
            callback(message);
        }
    }

private:
    std::unordered_map<CallbackHandle, Callback> callbacks_;
};

int main() {
    CallbackSubject subject;
    
    // 订阅
    auto handle1 = subject.subscribe(
        [](const std::string& msg) {
            std::cout << "Callback 1: " << msg << std::endl;
        }
    );
    
    auto handle2 = subject.subscribe(
        [](const std::string& msg) {
            std::cout << "Callback 2: " << msg << std::endl;
        }
    );
    
    // 通知
    subject.notify("First message");
    
    // 取消订阅
    subject.unsubscribe(handle2);
    
    // 再次通知
    subject.notify("Second message");
    
    return 0;
}