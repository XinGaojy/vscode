//深拷贝和浅拷贝


#if 0

#include <iostream>
#include <cstring>

class String {
public:
    char* data;
    String(const char* s = "") {
        data = new char[strlen(s) + 1];
        strcpy(data, s);
    }
    ~String() { delete[] data; }
};

int main() {
    String a("hello");
    String b = a;      // 默认拷贝构造：浅拷贝
    std::cout << a.data << " " << b.data << "\n";
    return 0;          // 结束时 double free → 未定义行为
}


#endif



#if 0

#include <iostream>
#include <cstring>

class String {
public:
    char* data;
    String(const char* s = "") {
        data = new char[strlen(s) + 1];
        strcpy(data, s);
    }
    // 深拷贝：自己分配新内存并复制内容
    String(const String& rhs) {
        data = new char[strlen(rhs.data) + 1];
        strcpy(data, rhs.data);
    }
    ~String() { delete[] data; }
};

int main() {
    String a("hello");
    String b = a;      // 深拷贝
    std::cout << a.data << " " << b.data << "\n";
    a.data[0] = 'H';   // 改 a 不影响 b
    std::cout << a.data << " " << b.data << "\n";
}

#endif

#if 0

#include <iostream>
#include <cstring>



int main() {
    std::string a = "hello";
    std::string b = a;   // std::string 自带深拷贝
    a[0] = 'H';
    std::cout << a << " " << b << "\n";   // Hello hello
}


#endif



#if 1
#include <vector>
#include <iostream>
int main() {
    std::vector<int> a(5, 1);
    std::vector<int> b = a;   // 深拷贝
    b[0] = 999;
    std::cout << a[0] << " " << b[0] << "\n";   // 1 999
}


#endif


