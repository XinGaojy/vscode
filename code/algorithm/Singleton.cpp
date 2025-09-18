
#if 0
#include<iostream>
using namespace std;
class Singleton{
private:
    //static Singleton*instance;
    Singleton()=default;
    ~Singleton(){
        //cout<<"distructor"<<endl;
    }
    Singleton& operator =(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
    void distructor(){
        cout<<"distructor"<<endl;
    }
public:
    static Singleton& getinstance(){
        static Singleton instance;
        return instance;
    }
    void print(){
        cout<<this<<endl;
    }
};


#endif




#if 0
#include<iostream>
#include<atomic>
#include<mutex>
using namespace std;
class Singleton{
private:
    static mutex mtx;
    static atomic<Singleton*>instance;
    Singleton()=default;
    ~Singleton()=default;
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
    static void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
        }
        //cout<<"distructor"<<endl;
    }
public:
    static Singleton*getinstance(){
        Singleton* temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(std::memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }

        return temp;
    }
    void print(){
        cout<<this<<endl;
    }
};
std::atomic<Singleton*>Singleton::instance{nullptr};
std::mutex Singleton::mtx;

#endif


// int main(){
//     Singleton::getinstance()->print();
//     return 0;   
// }





//实现一个单例模式

#if 0
#include<iostream>
#include<mutex>
using namespace std;
class Singleton{
private:
    Singleton()=default;
    ~Singleton()=default;
    Singleton& operator=(Singleton& )=delete;
    Singleton&operator=(Singleton&&)=delete;
    Singleton (const Singleton&)=delete;
    Singleton (Singleton&&)=delete;
public:
    static Singleton& getinstance(){
        static Singleton instance;
        return instance;
    }
    void print(){
        cout<<this<<endl;
    }
};
#endif






//实现双加锁
#if 0
#include<iostream>
#include<vector>
#include<atomic>
#include<mutex>
#include<unistd.h>
using namespace  std;
class Singleton{
private:
    static mutex mtx;

    static atomic<Singleton*>instance;
    Singleton()=default;
    ~Singleton()=default;
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&)=delete;
    static void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
            cout<<"distructor"<<endl;
        }
    }

public:
    void print(){
        cout<<this<<endl;
    }
    static Singleton*getinstance(){
        Singleton*temp=instance.load(memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,memory_order_release);
                atexit(distructor);
            }
        }
        return temp;
    }
};
mutex Singleton::mtx;
atomic<Singleton*> Singleton::instance;

int main(){
    Singleton::getinstance()->print();
    return 0;
}

#endif


#if 0
#include<iostream>
using namespace std;
class Singleton{
private:
    Singleton()=default;
    ~Singleton(){
        cout<<"distructor"<<endl;
    }
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;

public:
    static Singleton&getinstance(){
        static Singleton instance;
        return instance;

    }
    void print(){
        cout<<this<<endl;
    }
};
int main(){
    Singleton::getinstance().print();
    return 0;
}

#endif


#if 0


#include<iostream>
#include<atomic>

#include<unistd.h>
#include<mutex>
using namespace  std;
class Singleton{
private:
    static atomic<Singleton*> instance;
    static mutex mtx;
    Singleton()=default;
    ~Singleton(){
        cout<<"distructor"<<endl;
    };
    static void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
            cout<<"distructor"<<endl;
        }
    }
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
public:
    void print(){
        cout<<this<<endl;
    }
    static Singleton* getinstance(){
        Singleton*temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            unique_lock<mutex>lock(mtx);
            temp=instance.load(std::memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }
        return temp;
    }
};

std::atomic<Singleton*> Singleton::instance;
std::mutex Singleton::mtx;
int main(){
    Singleton::getinstance()->print();

}

#endif


#if 0
#include<iostream>
#include<vector>

using namespace std;
class Singleton{
private:
    Singleton()=default;
    ~Singleton(){
        cout<<"distructor"<<endl;
    }
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
public:
    static Singleton& getinstance(){
        static Singleton instance;
        return instance;

    }
    void print(){
        cout<<this<<endl;
    }
};
int main(){
    Singleton::getinstance().print();
    return 0;
}

#endif


#if 0
#include<iostream>
#include<atomic>
#include<mutex>
using namespace std;
class Singleton{
private:
    static atomic<Singleton*> instance;
    static mutex mtx;
    Singleton()=default;
    ~Singleton(){
        cout<<"hello world"<<endl;
    };
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    static void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
            cout<<"distructor"<<endl;
        }
    }
    
public:
    static Singleton* getinstance(){
        Singleton*temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            unique_lock<mutex>lock(mtx);
            temp=instance.load(std::memory_order_acquire);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }
        return temp;
    }
    void print(){
        cout<<this<<endl;
    }
};
atomic<Singleton*> Singleton::instance;
mutex Singleton::mtx;
int main(){
    Singleton::getinstance()->print();
    return 0;
}


#endif


#if 0
#include<iostream>
using namespace std;
class Singleton{
private:
    Singleton()=default;
    ~Singleton(){
        cout<<"distructor"<<endl;
    }
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
    
public:
    static Singleton& getinstance(){
        static Singleton instance;
        return instance;
    }
    void print(){
        cout<<this<<endl;
    }
}   
int main(){
    Singleton::getinstance().print();
    return 0;
}


class Singleton{
private:
    static atomic<Singleton*> instance;
    static mutex mtx;
    Singleton()=default;
    ~Singleton()=default;
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&&)=delete;
    void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
        }
    }
public:
    static Singleton* getinstance(){
        Singleton*temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(std::memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }

        }
    }
};

std::atomic<Singleton*> Singleton::instance;
std::mutex Singleton::mtx;


#endif



#if 0
#include<iostream>
using namespace std;
class Singleton{
private:
    Singleton()=default;
    ~Singleton(){
        cout<<"destructor"<<endl;
    }
    Singleton(const Singleton&)=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton&)=delete;
    Singleton& operator=(Singleton&& )=delete;

public:
    static Singleton& getinstance(){
        static Singleton instance;
        return instance;
    }
    void print(){
        cout<<this<<endl;
    }
};
int main(){
    Singleton::getinstance().print();
    return 0;
}

#endif


#if 0

#include<iostream>
#include<mutex>
#include<atomic>
#include <unistd.h>
using namespace std;
class Singleton{
private:
    static atomic<Singleton*> instance;
    static mutex mtx;
    Singleton()=default;
    ~Singleton(){
        cout<<"distructor"<<endl;
    };
    Singleton(const Singleton& )=delete;
    Singleton(Singleton&&)=delete;
    Singleton& operator=(const Singleton& )=delete;
    Singleton& operator=(Singleton&& )=delete;
    static void distructor(){
        if(instance!=nullptr){
            delete instance;
            instance=nullptr;
            //cout<<"distructor"<<endl;
        }
    }

public:
    static Singleton* getinstance(){
        Singleton* temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(std::memory_order_acquire);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }

        return temp;
    }
    void print(){
        cout<<this<<endl;
    }
};
atomic<Singleton*>Singleton::instance;
mutex Singleton::mtx;
int main(){
    Singleton::getinstance()->print();
}






Singleton(){
private:
    static atomic<Singleton*> instnace;
    static mutex mtx;
public:
    static Singleton* getinstance(){
        Singleton* temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(std::memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }
    }
    





    class Singleton{
    private:
        Singleton()=default;
        ~Singleton()=default;
        Singleton(const Singleton&  other)=delete;
        Singleton(Singleton&& other)=delete;
        Singleton& operator=(const Singleton&other)=delete;
        Singleton& operator=(Singleton&& other)=delete;
    public:
        static Singleton&  getinstance(){
            static Singleton instance;
            return instance;
        }
        void print(){
            cout<<this<<endl;
        }
    };

    int main(){
        Singleton::getinstance().print();
    }

class Singleton(){
private:
    static atomic<Singleton*> instance;
    static mutex mtx;
    Singleton()=default;
    ~Singleton()=default;
    Singleton(const Singleton& )=delete;
    Singleton(Singleton&& )=delete;
    Singleton& operator=(cosnt Singleton& )=delte;
    Singleton& operator=(Singleton&& )=delete;
public:
    static Singleton* getinstance(){
        Singleton* temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(std::memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singleton();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }

        }
        return temp;
    }
    void print(){
        cout<<this<<endl;
    }
};
atomic<Singleton*>Singleton::instance;
mutex Singleton::mtx;
int main(){
    Singleton::getinstance()->print();
    return 0;
}
#endif





//unique_ptr<FILE,decltype(&fclose)> fp(fopen("afile","r"),&fclose);
//if(fp==nullptr)exit();


//unique_ptr<FILE,decltype(&fclose)> fp(fopen("1.config",r),&fclose);






//unique_ptr<FILE,decltype(&fclose)> fp(fopen("file","r"), &fclose);
//


#include <iostream>
#include <cstdio>

using namespace std;

template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T instance;
        printf("Instance address: %p\n", &instance);  // 打印this指针地址
        return &instance;
    }
};

// 测试类
class Logger {
public:
    void log(const char* msg) {
        cout << "[Logger] " << msg << " (this=" << this << ")" << endl;
    }
};

int main() {
    // 获取单例并打印地址
    Logger* logger1 = Singleton<Logger>::GetInstance();
    logger1->log("First call");

    Logger* logger2 = Singleton<Logger>::GetInstance();
    logger2->log("Second call");

    // 验证地址相同
    printf("logger1=%p, logger2=%p\n", logger1, logger2);

    // 不同模板参数的测试
    Logger* altLogger = Singleton<Logger, int>::GetInstance();
    printf("altLogger=%p\n", altLogger);

    return 0;
}





