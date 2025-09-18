//String 
#if 0
#include <iostream>
#include <cstring>
#include <algorithm>
#include <stdexcept>

using namespace std;

class String {
public:
    String() : size_(0), capacity_(min_capacity) {
        data_ = new char[capacity_ + 1];
        data_[0] = '\0';
    }

    String(const char* str) {
        if (str == nullptr) {
            throw std::invalid_argument("nullptr");
        }
        size_ = strlen(str);
        capacity_ = std::max(size_, min_capacity);
        data_ = new char[capacity_ + 1];
        memcpy(data_, str, size_ + 1);
    }

    String(const void* data, size_t len) {
        if (data == nullptr) {
            throw std::invalid_argument("nullptr");
        }
        size_ = len;
        capacity_ = std::max(len, min_capacity);
        data_ = new char[capacity_ + 1];
        memcpy(data_, data, len);
        data_[len] = '\0';
    }

    String(const String& other) 
        : size_(other.size_), capacity_(other.capacity_) {
        data_ = new char[capacity_ + 1];
        memcpy(data_, other.data_, size_ + 1);
    }

    String(String&& other) noexcept 
        : data_(other.data_), 
          size_(other.size_), 
          capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
    }

    ~String() {
        delete[] data_;
    }

    String& operator=(const String& other) {
        if (this != &other) {
            char* new_data = new char[other.capacity_ + 1];
            memcpy(new_data, other.data_, other.size_ + 1);
            delete[] data_;
            data_ = new_data;
            size_ = other.size_;
            capacity_ = other.capacity_;
        }
        return *this;
    }

    String& operator=(String&& other) {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            realloc_data(new_capacity);
        }
    }

    void shrink_to_fit() {
        if (capacity_ > size_) {
            realloc_data(size_);
        }
    }

    String& append(const char* str, size_t len) {
        if (!str) {
            throw std::invalid_argument("nullptr");
        }
        if (size_ + len > capacity_) {
            reserve((size_ + len) * 2);
        }
        memcpy(data_ + size_, str, len);
        size_ += len;
        data_[size_] = '\0';
        return *this;
    }

    String& append(const char* str) {
        if (str == nullptr) {
            throw std::invalid_argument("nullptr");
        }
        return append(str, strlen(str));
    }

    const char* c_str() const noexcept { 
        return data_; 
    }
    
    const char* data() const noexcept { 
        return data_; 
    }
    
    size_t size() const noexcept { 
        return size_; 
    }
    
    size_t capacity() const noexcept { 
        return capacity_; 
    }
    
    bool empty() const noexcept { 
        return size_ == 0; 
    }

private:
    static const size_t min_capacity = 15;
    size_t size_;
    size_t capacity_;
    char* data_;

    void realloc_data(size_t new_capacity) {
        new_capacity = std::max(new_capacity, min_capacity);
        char* new_data = new char[new_capacity + 1];
        if (size_ > 0) {
            memcpy(new_data, data_, size_);
        }
        new_data[size_] = '\0';
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }
};

const size_t String::min_capacity;  // 静态成员变量定义

int main() {
    String s1;
    String s2("hello");
    String s3(s2);
    String s4(std::move(s2));
    
    s1 = s3;
    s1 = std::move(s4);
    cout << s1.data() << endl;
    
    s1.append(" world", 6);
    s1.append("!");
    cout << s1.c_str() << endl;
    
    return 0;
}
#endif









class MyString{
private:
    int size_;
    int capacity_;
    char* data_;
    static const int min_capacity=15;
    void realloc_data(int new_capacity){
        capacity_=max(capacity_,new_capacity);
        char* new_data_=new char[capacity_+1];
        memcpy(new_data_,data_,size_+1);
        delete []data_;
        data_=new_data_;
        
    }
public:
    MyString():size_(0),capacity_(min_capacity){
        data_=new char[capacity_+1];
        data[size_]='\0';
    }

    ~MyString(){
        delete [] data_;
    }

    explicit MyString(const void *str,int len){
        if(str==nullptr)std::throw_invalid("nullptr");
        capacity_=max(len,min_capacity);
        data_=new char[capacity+1];
        memcpy(data_,str,len);
        data_[len]='0';
    }
    MyString(const MyString&other){
        size_=other.size_;
        capacity_=other.capacity_;
        data_=new char[capacity_+1];
        memcpy(data_,other.data_,size_+1);
        
    }
    MyString(MyString&&other) noexcept{
        size_=other.size_;
        capacity_=other.capacity_;
        data_=other.data_;
        other.capacity_=0;
        other.size_=0;
        other.data_=nullptr;
    }
    MyString& operator=(const MyString&other){
        if(&other!=this){
            delete [] data_;
            data_=new char[other.capacity_+1];
            size_=other.size_;
            capacity_=other.capacity_;
            memcpy(data_,other.data_,size_+1);
            
        }
        return *this;
    }
    MyString& operator=(MyString&&other)noexcept{
        if(&other!=this){
            delete [] data_;
            size_=other.size_;
            capacity_=other.capacity_;
            data_=other.data_;
            other.size_=0;
            other.capacity_=0;
            other.data_=nullptr;
        }
        return *this;
    }

}







class MyString{
private:
    size_t size_;
    size_t capacity_;
    char *data_;
    static const size_t min_capacity_=15;
    void realloc(size_t new_capacity){
        capacity_=max(new_capacity,min_capacity);
        char*new_data_=new char[capacity_+1];
        memcpy(new_data_,data_,size_);
        delete data_[];
        data_=new_data_;
    }
public:
    MyString():size_(0),capacity(0){
        capacity_=min_capacity;
        data_=new char[capacity_+1];
        data_[0]='\0';
    }
    explicit MyString(const void* str,int len){
        if(str){
            capacity_=max(len,min_capacity);
            data_=new char[capacity_+1];
            size_=len;
            memcpy(data_,str,len);
            data_[len]='\0';
        }
    }
    MyString(const MyString& other){
        size_=other.size_;
        capacity_=other.capacity_;
        data_=new char[capacity+1];
        memcpy(data_,other.data_,size_+1);
    }
    
    MyString(MyString&& other)noexcept{
        size_=other.size_;
        capacity=other.capacity_;
        data_=other.data_;
        other.size_=nullptr;
        other.capacity=0;
        other.data_=nullptr;
    }
    MyString& operator=(const MyString& other){
        if(&other!=this){
            delete [] data_;
            size_=other.size_;
            capacity=other.capacity_;
            data_=new char[capacity_+1];
            memcpy(data_,other.data_,size_+1);

        }
        return *this;
    }
    MyString&  operator=(MyString&& other)noexcept{
        if(&other!=this){
            size_=other.size_;
            capacity=other.capacity_;
            data_=other.data_;
            other.size_=0;
            other.capacity_=0;
            other.data_=nullptr;
        }
        return *this;
    }
    void reserve(size_t new_capacity){
        realloc(new_capacity);
    }
    MyString& append(const char*str,size_t len){
        if(len+size_>capacity_){
            reserve((len+size)*2);
        }
        memcpy(data_+size_,str,len);
        size_=size_+len;
        data_[size_]='\0';
        return *this;
    }
    
}