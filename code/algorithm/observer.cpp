
#if 0
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


#endif



#if 0


#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

// ==================== 观察者模式实现 ====================
class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const std::string& message) = 0;
};

class Subject {
private:

    std::string state_;
    std::vector<Observer*>observers_;
public:
    void attach(Observer* observer) {
        observers_.push_back(observer);
    }
    
    void detach(Observer* observer) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    }
    
    void setState(const std::string& newState) {
        state_ = newState;
        notify();
    }
    
    void notify() {
        for (auto observer : observers_) {
            observer->update(state_);
        }
    }
};

class Phone : public Observer {
private:
    std::string name_;
    
public:
    Phone(const std::string& name) : name_(name) {}
    
    void update(const std::string& message) override {
        std::cout << "📱 " << name_ << " 收到: " << message << std::endl;
    }
};

class Computer : public Observer {
private:
    std::string name_;
    
public:
    Computer(const std::string& name) : name_(name) {}
    
    void update(const std::string& message) override {
        std::cout << "💻 " << name_ << " 收到: " << message << std::endl;
    }
};


void testObserverPattern() {
    std::cout << "🎯 === 测试观察者模式 ===" << std::endl;
    
    Subject newsPublisher;
    
    Phone iphone("iPhone14");
    Phone android("华为手机");
    Computer macbook("MacBook");
    
    // 注册观察者
    newsPublisher.attach(&iphone);
    newsPublisher.attach(&android);
    newsPublisher.attach(&macbook);
    
    // 发布消息，所有观察者自动收到通知
    newsPublisher.setState("🚀 新版本发布啦！");
    std::cout << std::endl;
    
    // 移除一个观察者
    newsPublisher.detach(&android);
    newsPublisher.setState("🎉 周末大促销！");
    std::cout << std::endl;
}
// ==================== 主函数 ====================
int main() {
    std::cout << "🌟 设计模式演示 🌟" << std::endl << std::endl;
    
    // 测试观察者模式
    testObserverPattern();
    std::cout << std::endl;
    
    // 测试策略模式
  //  testStrategyPattern();
    
    return 0;
}





#endif


#if 0
#include<iostream>
#include<vector>
#include<string>
using namespace std;


class observer{
private:
    observer();
    virtual ~observer();
    virtual void update(const string &s)=0;
};

class subject{
private:
    std::vector<observer*>observers_;
    std::string state_;
public:
    void push(observer* o){
        observers_.push_back(o);

    }

    void pop(observer* o){
        observers_.erase(remove(observers_.begin(),observers_.end(),o),observers_.end());

    }

    void setstate(const string &state){
        state_=state;
        notify();
    
    }

    void notify(){
        for(auto i:observers_){
            i->update(state_);
        }
    }

};

class Phone :public observer{
private:
    std::string name_;

public:
    Phone(const string & name):name_(name){}

    void update(const std::string &message)override{
        std::cout<<message<<endl;
    };



};

class Computer:public Observer{
private:
    string name_;
public:
    Computer(const string & name):name_(name){}

    void update(const string & message)observer{
        cout<<name_<<message<<endl;
    }

};


int main()
{
    subject sub;
    Phone iphone("iPhone14");
    Computer macbook("MacBook14");
    sub.push(&iphone);
    sub.push(&macbook);
    sub.setState("新版本");
    sub.pop(&iphone);
}


#endif








#include<vector>
#include<string>
#include<algorithm>
#include<iostream>
using namespace std;
class base{
public:
    virtual ~base()=default;
    virtual void update(const string& name)=0;
};

class iphone: public base{
private:
    string name_;
public:
    iphone(const string& s):name_(s){}
    void update(const string&name) override {
        cout<<name_<<name<<endl;    
    }
};

class macbook: public base{
private:
    string name_;
public:
    macbook(const string s):name_(s){}
    void update(const string&name) override {
        cout<<name_<<name<<endl;
    }
};

class control{
private:
    string name_;
    vector<base*>vec_;
public:
    void push(base* o){
        vec_.push_back(o);
    }

    void remove(base* o){
        vec_.erase(std::remove(vec_.begin(),vec_.end(),o),vec_.end());
    }
    
    void setstate(const string&new_name){
        name_=new_name;
        notify();
    }
    void notify(){
        for(auto i:vec_){
            i->update(name_);
        }
    }
};

int main(){
    control ctrl;
    iphone ip("1");
    macbook mc("2");
    ctrl.push(&ip);
    ctrl.push(&mc);
    ctrl.setstate("hello");
    ctrl.remove(&ip);
    ctrl.setstate("bage");
    return 0;
}
