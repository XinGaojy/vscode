#if 0

#include<iostream>
#include<stdlib.h>
#include<cstdio>
#include<utility>
using namespace std;


#if 0
class file{
private:
  std::FILE *fd;

public:

};

#endif




int main(){

  FILE* fd=fopen("log.txt","w");
  fwrite("helloworld",1,10,fd);
//  fclose(fd);
  return 0;
}


#endif


#if 0
#include <cstdio>
#include <utility>      // std::exchange

#include<iostream>
#include<stdlib.h>
#include<cstdio>
#include<utility>
using namespace std;


class File {
public:
    /* 构造：打开文件 */
    explicit File(const char* path, const char* mode = "w")
        : fp_(std::fopen(path, mode)) {
        if (!fp_) throw std::runtime_error("fopen failed");
    }

    /* 移动构造 */
    File(File&& other) noexcept : fp_(std::exchange(other.fp_, nullptr)) {}

    /* 移动赋值 */
    File& operator=(File&& other) noexcept {
        if (this != &other) {
            close();
            fp_ = std::exchange(other.fp_, nullptr);
        }
        return *this;
    }

    /* 析构：自动关闭 */
    ~File() { close(); }

    /* 写入接口示例 */
    std::size_t write(const void* data, std::size_t size) {
        return std::fwrite(data, 1, size, fp_);
    }

    /* 手动关闭（重复关安全）*/
    void close() noexcept {
        if (fp_) {
            std::fclose(fp_);
            fp_ = nullptr;
        }
    }

    /* 删除拷贝 */
    File(const File&) = delete;
    File& operator=(const File&) = delete;

private:
    std::FILE* fp_;
};

/* -------------- 使用示例 -------------- */
#include <iostream>

int main() {
    try {
        File log("log.txt","w");
        const char msg[] = "RAII file write test\n";
        log.write(msg, sizeof(msg) - 1);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;   // 离开作用域自动 fclose
}

#endif







#if 0
#include<iostream>
#include<stdlib.h>

#include<iostream>
#include<stdlib.h>
#include<cstdio>
#include<utility>
using namespace std;


using namespace std;
class FileManager{
private:
    FILE* fd;
    
public:
   explicit FileManager(const char* text,const char* mode):fd(fopen(text,mode)){
        if(!fd){
            throw runtime_error("bad_error");
        }
   }

   ~FileManager(){
        close();
   }

   void close() noexcept{
        if(!fd){
            fclose(fd);
            fd=nullptr;
        }
   }
    
   size_t write(const void* data,const size_t size){
        return fwrite("helloworld",1,size,fd);
   }
   
   FileManager(const FileManager& )=delete;
   FileManager& operator=(const FileManager& )=delete;
   //File(File&& other) noexcept : fp_(std::exchange(other.fp_, nullptr)) {}
   

   FileManager(FileManager&& other) noexcept{
    #if 0
       fd=other.fd;
       other.fd=nullptr;
    #endif

       fd=std::exchange(other.ptr,nullptr);
   }


   FileManager& operator=(FileManager&& other)noexcept{
        
#if 0
       if(this!=&other){
            close();
            fd=other.fd;
            other.fd=nullptr;
        }
#endif
        if(this!=&other){
            close();
            fd=exchange(other.fd,nullptr);
        }
        return *this;
   }


};
int main(){
    FileManager file("log.txt","w");
    const char msg[] = "RAII file write test\n";
    file.write(msg,sizeof(msg)-1);
    return 0;
}


#if 0
int main() {
    try {
        File log("log.txt","w");
        const char msg[] = "RAII file write test\n";
        log.write(msg, sizeof(msg) - 1);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;   // 离开作用域自动 fclose
}

#endif



#endif

#if 0
#include <cstdio>
#include <memory>
#include <utility>
#include<iostream>
#include<stdlib.h>
#include<cstdio>
#include<utility>
using namespace std;


class File {
public:
    /* 1. 构造：打开文件 */
    explicit File(const char* path, const char* mode = "w")
        : fp_(std::fopen(path, mode), &File::deleter) {
        if (!fp_) throw std::runtime_error("fopen failed");
    }

    /* 2. 移动构造 */
    File(File&&) noexcept = default;

    /* 3. 移动赋值 */
    File& operator=(File&&) noexcept = default;

    /* 4. 析构：unique_ptr 自动调用 deleter */
    ~File() = default;

    /* 5. 用户接口：写数据 */
    std::size_t write(const void* data, std::size_t size) {
        return std::fwrite(data, 1, size, fp_.get());
    }

    /* 6. 手动关闭（重复关安全）*/
    void close() noexcept { fp_.reset(); }

private:
    /* 7. 自定义 deleter = fclose */
    static void deleter(std::FILE* fp) {
        if (fp) std::fclose(fp);
    }

    /* 8. 资源载体：unique_ptr 接管 FILE* */
    std::unique_ptr<std::FILE, decltype(&deleter)> fp_;

    /* 9. 禁止拷贝 */
    File(const File&) = delete;
    File& operator=(const File&) = delete;
};

/* ---------------- 使用示例 ---------------- */
#include <iostream>

int main() {
    try {
        File log("log.txt");
        const char msg[] = "unique_ptr + custom deleter\n";
        log.write(msg, sizeof(msg) - 1);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;          // 离开作用域 unique_ptr 自动 fclose
}



#endif







#if 0
#include<iostream>
#include<memory>
#include<iostream>
#include<stdlib.h>
#include<cstdio>
#include<utility>
using namespace std;



using namespace std;
class FileManager{
public:
    
    FileManager(const char *file_name,const char* mode="w"):fd(fopen(file_name,mode),&FileManager::deleter){
        if(!fd){
            throw runtime_error("fopen failed");
        }
    }

    ~FileManager()=default;
    
    size_t write(const void * data,std::size_t size){
        return std::fwrite(data,1,size,fd.get());
    }

    void close(){
        fd.reset(); 
#if 0
        if(fd){
            delete fd;
            fd=nullptr;
        }
#endif

    }

private:
    static void deleter(FILE* fp){
        if(fp){
            fclose(fp);
            fp=nullptr;
        }
    }
    
    unique_ptr<FILE,decltype(&deleter)>fd;
    FileManager(const FileManager&)=delete;
    FileManager& operator=(const FileManager&)=delete;


};
int main(){
    FileManager file("log.txt","w");
    file.write("helloworld",10);

    return 0;
}
#endif


