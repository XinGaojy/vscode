


#include<iostream>
using namespace std;
class Myvector{
private:
    int size_=0;
    int capacity_=0;
    int *data_;
    void realloc(int new_capacity){
        capacity_=new_capacity;
        char *new_data_=new int[capacity_];
        copy(new_data_,data_,size_);
        delete []data_;
        data_=new_data_;

    }
public:
    Myvector(int n){
        size_=0;
        capacity_=n;
        data_=nullptr;

    }
    ~Myvector(){
        size_0;
        capacity_=0;
        data_=nullptr;
    }
    Myvector(const Myvector& other){
        size_=other.size_;
        capacity_=other.capacity_;
        data_=new int[capacity_];

    }
    Myvector(Myvector&& other)noexcept{
        size_=other.size_;
        capacity_=other.capacity_;
        data_=other.data_;
        other.size_=0;
        other.capacity_=0;
        ohter.data_=nullptr;
    }
    Myvector& operator=(const Myvector& other){
        if(&other!=this){
            delete [] data_;
            size_=other.size_;
            capacity_=other.capacity_;
            data_=new int[capacity_+1];
            copy(data_,other.data_,size_);
            
        }
        return *this;
    }
    Myvector& operator=(Myvector&& other)noexcept{
        if(&other!=this){
            delete []data_;
            size_=other.size_；
            capacity_=other.capacity_;
            data_=other.data_;
            other.size_=0;
            other.capacity_=0;
            other.data_=nullptr;
        }
        return *this;
    }
    void reserve(int new_capacity){
        realloc(new_capacity);
    }
    void shrink_to_fit(){
        reserve(size_);
    }
    void push_back(int val){
        if(size==capacity_){
            reserve(size_==0?1:capacity_*2);
        }
        data_[size_++]=std::move(val);
    }
    template<class... Args>
    void emplace_back(Args... args){
        if(size_==capacity_){
            reserve(size_==0?1:2*capacity_);

        }
        new (&data_[size_++]) int(forward<Args>(args)...);
        //new (data_+size_++) int(forward<Args>(args)...);
    }
};
int main(){
    
}
