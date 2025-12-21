
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

#if 0
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


#endif





#if 0

#include<iostream>
using namespace std;
template<class T,int N>
class Lockfreedeque{
private:
    struct element{
        T *data_;
        atomic<bool> full_;
    };
    vector<element>vec;
    atomic<size_t>read_index_;
    atomic<size_t>write_index_;
    condition_variable condition;
public:
    Lockfreedeque(int N){
        read_index.store(0,std::memory_ordered_relaxed);
        write_index_.store(0,std::memory_ordered_relaxed);
        for(auto i:vec){
            i.full_.store(false,std::memory_ordered_relaxed);
        }
    }
    bool enqueue( T *x){
       // size_t temp=write_index_.load(std::memory_order_acuire);
        element e;

        do {

            size_t temp=write_index_.load(std::memory_order_acuire);
            if(read_index_.load(std::memory_ordered_acquire) + vec.size()>write_index)
            {
                return false;
            }

            size_t index=temp% vec.size();
            e=vec[index];
            if(e.full_.load(std::memory_order_relaxed){
                return false;
            }

        }while(!write_index_.compare_exchange_weak(
                    write_index_,
                    write_index_+1,
                    std::memory_release,
                    std::memory_ordered_relaxed);

            vec[index].data_=std::move(x);
            vec[index].full_.store(true,std::memory_ordered_release);
            write_index_.store(temp+1,std::memory_ordered_release);
            return true;
    }

    bool dequeue(T & x){
        element e;
        do {
            size_t temp=read_index_.load(std::memory_ordered_acquire);
            if(temp>write_index_.load(sd::memory_acquire){
                    return false;
            }
            
            e=std::move(vec[temp]);
            if(!e.full_.load(std::memory_ordered_relaxed){
                return false;
            }
            
            


        }while(!read_idnex_.compare_exchange_weak(temp,
                    temp+1,
                    std::memory_order_relaxed,
                    std::memory_ordered_release);

            x=std::move(e.data);
            e.full_.store(false,std::memory_ordered_release);
            return true;
    }
};

int main(){

    return 0;
}







#endif







#if 0

#include<iostream>
using namespace std;
class Singletion{
private:
    mutex mtx;
    Singletion()=default;
    Singletion(const Singletion & )=delete;
    Singletion(Singletion&& )=delete;
    Singletion& operator=(const Singletion& )=delete;
    Singletion& operator=(const Singletion&& )=delete;
    static Singletion& getinstance(){
        static Singletion instance;
        return instance;
    }

public:
    void print(){
        cout<< this<<endl;
    }
};


#endif

#include<iostream>
#include<mutex>
#include<atomic>
#include<unistd.h>
#include<memory>
using namespace std;


class Singletion{
private:
    atomic<Singletion*>instance;
    mutex mtx;
   static  void distructor(){
        if(instance){
            delete instance;
            instance==nullptr;

        }
        
    }
    ~Singletion()=delete;
    Singletion()=default;
    Singletion(const Singletion& )=delete;
    Singletion&  operator=(const Singletion& )=delete;
    Singletion(const Singletion&& )=delete;
    Singletion& operator=(Singletion&& )=delete;

public:    
    void print(){
        cout<<this<<endl;
    }
    static Singletion* getinstance(){
        Singletion* temp=instance.load(std::memory_order_acquire);
        if(temp==nullptr){
            lock_guard<mutex>lock(mtx);
            temp=instance.load(memory_order_relaxed);
            if(temp==nullptr){
                temp=new Singletion();
                instance.store(temp,std::memory_order_release);
                atexit(distructor);
            }
        }
        return temp;
    }

};

 static atomic<Singletion*>Singletion::instance;
static  mutex Singletion::mtx;

int main(){
#if 0
    getinstance.print();
#endif

    Singletion::getinstance()->print();
    return 0;
}








