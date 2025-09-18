
#if 0
#include<chrono>
#include<iostream>
#include<mutex>
#include<vector>
#include<condition_variable>
#include<functional>
#include<atomic>
#include<queue>
#include<thread>
using namespace std;
class ThreadPool{
private:
    struct task{
        int id;
        function<void()>func;
        task(int i,function<void()>f):id(i),func(f){}
    };
    vector<thread>threads;
    queue<task>tasks;
    bool stop;
    condition_variable condition;
    mutex mtx;
    atomic<int>next_id{0};
    atomic<int>current_id{0};
public:
    ThreadPool(int n):stop(false){
        for(int i=0;i<n;i++){
            threads.emplace_back([this]{
                while(1){
                    task tk(-1,nullptr);
                    {
                        unique_lock<mutex>lock(mtx);
                        condition.wait(lock,[this]{
                            return stop||(!tasks.empty()&&tasks.front().id==current_id);
                        });
                        if(stop&&tasks.empty()){
                            return ;
                        }
                        {
                            //unique_lock<mutex>lock(mtx);
                            if(tasks.front().id==current_id){
                                tk=std::move(tasks.front());
                                tasks.pop();
                    
                            }
                            
                        }
                        
                    }
                    tk.func();
                    {
                        unique_lock<mutex>lock(mtx);
                        current_id++;
                    }
                    condition.notify_all();
                }
            });
        }
    }
    template<class T,class... Args>
    void enqueue(T&&t,Args... args){
        function<void()>func=std::bind(std::forward<T>(t),std::forward<Args>(args) ...);
        {
            unique_lock<mutex>lock(mtx);
            //tasks.emplace(std::move(task));
            tasks.emplace(task{next_id++,func});
        }
        condition.notify_one();
    }
    ~ThreadPool(){
        {
            unique_lock<mutex>lock(mtx);
            stop=true;
        }    
        condition.notify_all();
        for(auto &i:threads){
            if(i.joinable()){
                i.join();
            }
        }
    }
};
int main(){
    ThreadPool tp(4);
    for(int i=0;i<100;i++){
        tp.enqueue([i]{
            cout<<i<<endl;
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}



#endif




#if 0
#include<iostream>
#include<condition_variable>
#include<vector>
#include<mutex>
#include<atomic>
#include<queue>
#include<thread>
#include<chrono>

using namespace std;
class ThreadPool{
private:
    struct task{
        int id;
        function<void()>func;
        task(int i,function<void()>f):id(i),func(f){}
    };
    vector<thread>threads;
    queue<task>tasks;
    mutex mtx;
    condition_variable condition;
    bool stop;
    atomic<int>current_id{0};
    aotmic<int>next_id{0};
public:
    
    ThreadPool(int n){
        for(int i=0;i<n;i++){
            threads.emplace_back([this]{
                while(1){
                    task tk{-1,nullptr};
                    {
                        unique_lock<mutex>lock(mtx);
                        condition.wait(lock,[this]{
                            return stop||(!tasks.empty()&&curent_id==tasks.front().id);
                        });
                        if(stop&&tasks.empty()){
                            return ;
                        }
                        tk=std::move(tasks.front());
                        tasks.pop();
                    }
                    tk.func();
                    {
                        unique_lock<mutex>lock(mtx);
                        current_id++;
                    }
                    condition.notify_all();
                }
            });
        }
    }
    ~ThreadPool(){
        {
            unique_lock<mutx>lock(mtx);
            stop=true;
        }
        condition.notify_all();
        for(auto i:threads){
            if(i.joinable()){
                i.join();
            }
        }
    }
    template<class T,class... Args>
    void enqueu<T &&t,Args&&... args>
    {
        function<void()>func=bind(std::forward<T>t,std::forward<Args>(args)...));
        {
            unique_lock<mutex>lock(mtx);
            tasks.emplace(task{next_id++,func});
        }
        condition.notify_one();
    }
}
int main(){
    ThreadPool tp(4);
    for(int i=0;i<100;i++){
        tp.enqueue([i]{
            cout<<i<<endl;
        });
    }
    std::chrono::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}

#endif




#if 0

#include<iostream>
#include<atomic>
#include<thread>
#include<mutex>
#include<condition_variable>
using namespace std;
atomic<int>current_id{0};
mutex mtx;
condition_variable condition;
const int nummax=100;
void printnum(int thread_id,int nummax){
    for(int i=thread_id;i<nummax;i+=8){
        
            unique_lock<mutex>lock(mtx);
            condition.wait(lock,[i]{
                return i==current_id;
            });
            cout<<"thread"<<thread_id<<"num"<<i<<endl;
            current_id++;
            
        
        condition.notify_all();
    }
}
int main(){
    
    thread t1(printnum,0,nummax);
    thread t2(printnum,1,nummax);
    thread t3(printnum,2,nummax);
    thread t4(printnum,3,nummax);
    thread t5(printnum,4,nummax);
    thread t6(printnum,5,nummax);
    thread t7(printnum,6,nummax);
    thread t8(printnum,7,nummax);
//    thread t2(printnum,1,nummax);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    return 0;
}


#endif





#if 0
#include<iostream>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<thread>

using namespace  std;
mutex mtx;
const int maxnum=100;
condition_variable condition;
atomic<int>current_id(0);
void printnum(int thread_id,int maxnum){
    for(int i=thread_id;i<maxnum;i+=3){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[i]{
            return i==current_id;
        });
        cout<<i<<endl;
        current_id++;
        condition.notify_all();
    }
}
char current_char='a';
void printchar(char c,char next_char){
    for(int i=0;i<maxnum;i++){
        unique_lock<mutex>lock(mtx);
        condition.wait(lock,[c]{
            return c==current_char;
        });
        cout<<c<<endl;
        current_char=next_char;
        condition.notify_all();
    }
}
int main(){

    thread t1(printchar,'a','b');
    thread t2(printchar,'b','c');
    thread t3(printchar,'c','a');
    t1.join();
    t2.join();
    t3.join();
    return 0;
}

#endif











#include<iostream>
using namespace std;
class ThreadPool{
private:
    condition_variable condition;
    vector<thread>threads;
    mutex mtx;
    struct task{
        int id;
        function<void()>func;
    };
    queue<task>tasks;
    atomic<int>currrent_id{0};
    atomic<int>next_id{0};
public:
    ThreadPool(int n):threads(n){
        for(int i=0;i<n;i++)
        {
            threads.emplace_back([this]{
                
                while(1){
                    task tk{-1,nullptr};
                    {
                        unique_lock<mutex>lock(mtx);
                        condition.wait(lock,[this]{
                            return stop||(!tasks.empty()&&tasks.front().id==current_id);
                        }); 
                        if(stop&&tasks.empty()){
                            return ;
                        }
                        tk=std::move(tasks.front());
                        tasks.pop();
                    }
                    tk.front().func();
                    {
                        unique_lock<mutex>lock(mtx);
                        currrent_id++;

                        
                    }
                    condition.notify_all();
                }
            });
        }
    }
    ~ThreadPool(){
        {
            unique_lock<mutex>lock(mtx);
            stop=true;

        }
        condition.notify_all();
        for(auto &i:threads){}
        {
            if(i.joinable()){
                i.join();
            }
        }
    }
    template<class T,class... Args>
    void enqueue(T && t,Args... args){
        function<void()>func=std::bind(std::forward<T>t,std::forward<Args>(args)...);
        {
            unique_lock<mutex>lock(mtx);
            tasks.emplace(task{next_id++,func});

        }
        condition.notify_one();
    }
};
int main(){
    ThreadPool tp(4);
    for(int i=0;i<100;i++){
        tp.enqueue([i]{
            cout<<i<<endl;
        })
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}














#include<iostream>
using namespace std;
class  ThreadPool{
private:
    struct task{
        int id;
        function<void()>func;
    };
    vector<thread>threads;
    queue<task>que;
    condition_variable condtion;
    mutex mtx;
    bool stop;
    atomic<int>current_id{0};
    aotmic<int>next_id{0};
public:
    ThreadPool(int n):stop(false){
        for(int i=0;i<n;i++){
            threads.emplace_back([this]{
                while(1){
                    task tk{-1,nullptr};
                    {
                        unique_lock<mutex>lock(mtx);
                        condition.wait(lock,[this]{
                            return stop||(!tasks.empty()&&current_id==tasks.front().id);
                        })
                        if(current_id==tasks.front().id){
                            tk=tasks.front();
                            tasks.pop();
                        }
                    }
                    tk.func();
                    {
                        unique_lock<mute>lock(mtx);
                        current_id++;
                        
                    }
                    conditon.notify_all();
                }
            });
        }
        ~ThreadPool(){
            {
                unique_lock<mutex>lock(mtx);
                stop=true;
            }
            condition.notify_all();
            for(auto &i:threads){
                if(i.joinable()){
                    i.join();
                }
            }
        }
        template<class T,class... Args>
        void enqueue(T&& t,Args... args){
            function<void()>func=std::bind(std::forward<T>t ,std::forward<Args>(args)...);
            {
                unique_lock<mutex>lock(mtx);
                tasks.emplace{next_id++,func};
            }
            condition.notify_one();
        
        }
    }
}
int main(){
    ThreadPool tp(4);
    for(int i=0;i<100;i++){
        tp.enqueue([i]{
            cout<<i<<endl;
        });
    }
    return 0;
}





#include<iostream>
using namespace std;
class threadpool{
private:
	struct task{
		int id;
		function<void()> func;
		task(int i,function<void()> f):id(i), func(f) {}
	};
	vector<thread>threads;
	variable_conditoin condition;
	deque<task>tasks;
	mutex mtx;
	atomic<int> current_id={0};
	aotmic<int> next_id={0};
	bool stop;
};
public:
threadpool(int n){
	for(int i=0; i<n; i++){
		threads.emplace([this]{
			task tk={-1,nullptr};
			while(1){
				{
					unique_lock<mutex>lock(mtx);
					condition.wait([this]{
						return !stop ||(!tasks.empty() && tasks.front().id == id);		
					});
					if(stop&& tasks.empty()){
						return ;
					}
					tk=std::move(tasks.front());
					tasks.pop_front();
				}
				if(tk.func()){
					tk.func();
				}
				{
					unqiue_lock<mutex> lock(mtx);
					current_id++;	
				}
				condition.notify_one();
			}		
		});		
	}
~threadpool(){
	{
		unique_lock<mutex>lock(mtx);
		stop=false;

	}
	condition.notify_all();
	for(auto i: threads){
		if(i.joinable()){
			i.join();
		}
	}
	
}

template<class T, class... Args>
bool enqueue(T && t,Args... args){
	function<void()> f=std::bind(std::forward<T>& t,std::forward<Args>(&args)...);
	{
		unique_lock<mutx>lock(mtx);
		tasks.empalce(task{f, next_id++});
	}
	condition.notify_one();

}

};

int main(){
	threadpool tp;
	for(int i=0; i<10; i++){
		tp.enqueue([i]{});
	}
	return 0;
}






#include<iostream>
using namespace std;
class threadpool {
private:
	struct task{
		int id;
		function<void()>func;
		task(int i,fucntion<void>f):id(i),func(f){}
	};
	vector<thread>threads;
	variable_condtion condition;
	mutex mtx;
	deque<task>tasks;
	bool stop;
	atomic<int>current_id={0};
	aotmic<int>next_id={0};
public:
	threadpool(int n):stop(false){
		for(int i=0;i<n;i++){
			threads.emplace([this]{
				while(1){
					task tk{-1,nullptr};
					{
						unique_lock<mutex>lock(mtx);
						condtion.wait([this]{
							return stop|| (!tasks.empty()&& current_id!=tasks.front().id; 		
						});
						if(stop&& tasks.empty()) return ;
						tk=std::move(tasks.front());
						tasks.pop();
					}	
					tk.func();
					current_id++;
					condition.notify_one();
				}		
			});
		}
	}
	~threadpool(){
		{
			unique_lock<mutex>lock(mtx);
			stop=false;
		}
		condition.notify_all();
		for(auto i:threads){
			if(i.joinable()){
				i.join();
			}
		}

	}
	template<class T,class... Args>
	void enqueue(T && t,Args && args>
	{
		function<void()> func=std::bind(std::forward<T>& t,std::forward<Args>(&args)...);
		{
			unique_locke<mutex>lock(mtx);
			tasks.emplace{task{next_id++,func}};
		}
		condition.notify_one();
	}	

};
int main(){
	for(int i=0;i< 10;i++){
		threads.emplace_back([i]{
			cout<<i<<endl;		
		});
	}
	return 0;

}
