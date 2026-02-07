#include <iostream>
#include <cstring>
#include <iomanip>

// 结构体示例
struct Employee {
    int id;         // 4字节
    float salary;   // 4字节
    char name[20];  // 20字节
    bool active;    // 1字节（对齐为4字节）
    char stu[];
    // 总计约 32字节（考虑对齐）
};

// 联合体示例
union Data {
    int i;          // 4字节
    float f;        // 4字节
    char str[20];   // 20字节
    // 总计 20字节（最大成员大小）
};

void demonstrate_basics() {
    std::cout << "=== 基本示例 ===" << std::endl;
    
    // 结构体
    Employee emp;
    emp.id = 1001;
    emp.salary = 5000.50f;
    strcpy(emp.name, "张三");
    emp.active = true;
    
    std::cout << "结构体 Employee:" << std::endl;
    std::cout << "  ID: " << emp.id << std::endl;
    std::cout << "  薪资: " << emp.salary << std::endl;
    std::cout << "  姓名: " << emp.name << std::endl;
    std::cout << "  状态: " << (emp.active ? "活跃" : "不活跃") << std::endl;
    std::cout << "  大小: " << sizeof(Employee) << " 字节" << std::endl;
    
    std::cout << "\n---\n" << std::endl;
    
    // 联合体
    Data data;
    data.i = 42;
    std::cout << "联合体 Data 存储 int: " << data.i << std::endl;
    
    data.f = 3.14f;
    std::cout << "联合体 Data 存储 float: " << data.f << std::endl;
    // 注意：现在 data.i 的值已被覆盖！
    std::cout << "此时 data.i 的值（被覆盖）: " << data.i << " (未定义行为)" << std::endl;
    
    strcpy(data.str, "Hello");
    std::cout << "联合体 Data 存储 string: " << data.str << std::endl;
    std::cout << "联合体大小: " << sizeof(Data) << " 字节" << std::endl;
}


#include <iostream>
#include <cstdint>

void visualize_memory_layout() {
    std::cout << "\n=== 内存布局对比 ===" << std::endl;
    
    // 结构体
    struct Point3D {
        float x;    // 偏移 0
        float y;    // 偏移 4
        float z;    // 偏移 8
    };
    
    // 联合体
    union Number {
        int integer;        // 偏移 0
        float real;         // 偏移 0
        char bytes[4];      // 偏移 0
    };
    
    Point3D point = {1.0f, 2.0f, 3.0f};
    Number num = {0x12345678};
    
    std::cout << "结构体 Point3D 布局:" << std::endl;
    std::cout << "  &point.x = " << (void*)&point.x 
              << "，值 = " << point.x << std::endl;
    std::cout << "  &point.y = " << (void*)&point.y 
              << "，值 = " << point.y << std::endl;
    std::cout << "  &point.z = " << (void*)&point.z 
              << "，值 = " << point.z << std::endl;
    std::cout << "  sizeof(Point3D) = " << sizeof(Point3D) << std::endl;
    
    std::cout << "\n联合体 Number 布局:" << std::endl;
    std::cout << "  &num.integer = " << (void*)&num.integer << std::endl;
    std::cout << "  &num.real    = " << (void*)&num.real << std::endl;
    std::cout << "  &num.bytes[0] = " << (void*)&num.bytes[0] << std::endl;
    std::cout << "  sizeof(Number) = " << sizeof(Number) << std::endl;
    
    std::cout << "\n演示联合体共享内存:" << std::endl;
    num.integer = 0x12345678;
    
    std::cout << "设置 integer = 0x" << std::hex << num.integer << std::dec << std::endl;
    std::cout << "字节表示: ";
    for (int i = 0; i < 4; ++i) {
        std::cout << std::hex << (int)(uint8_t)num.bytes[i] << " ";
    }
    std::cout << std::dec << std::endl;
    
    num.bytes[0] = 0xAA;
    std::cout << "修改 bytes[0] = 0xAA" << std::endl;
    std::cout << "现在 integer = 0x" << std::hex << num.integer << std::dec << std::endl;

    
    std::cout << "现在 float:" << num.real << std::endl;
}


#include <iostream>
#include <string>
#include <vector>

struct AdvancedStruct {
    // 1. 位域
    unsigned int flag1 : 1;    // 1位
    unsigned int flag2 : 2;    // 2位
    unsigned int : 5;           // 5位填充
    unsigned int value : 8;     // 8位
    
    // 2. 灵活数组成员（C99特性，C++中不推荐）
    // int data[];  // 必须是最后一个成员
    
    // 3. 匿名结构体
    struct {
        int x;
        int y;
    } point;
    
    // 4. 内联成员初始化（C++11）
    int id = 0;
    std::string name = "Unknown";
    
    // 5. 方法
    void print() const {
        std::cout << "ID: " << id << ", Name: " << name << std::endl;
    }
    
    // 6. 构造函数
    AdvancedStruct() = default;
    AdvancedStruct(int i, const std::string& n) : id(i), name(n) {}
    
    // 7. 析构函数
    ~AdvancedStruct() {
        std::cout << "销毁 AdvancedStruct: " << name << std::endl;
    }
    
    // 8. 静态成员
    static int count;
    
    // 9. 友元函数
    friend void access_private(AdvancedStruct& s);
    
private:
    // 10. 访问控制
    int secret = 42;
};

int AdvancedStruct::count = 0;

void access_private(AdvancedStruct& s) {
    std::cout << "访问私有成员: " << s.secret << std::endl;
}

// 嵌套结构体
struct Company {
    struct Department {
        std::string name;
        int employee_count;
        
        struct Team {
            std::string leader;
            int member_count;
        } teams[5];
    };
    
    std::string company_name;
    Department departments[10];
    
    // 结构体中的联合体
    union {
        int established_year;
        long foundation_timestamp;
    } foundation;
};

void demonstrate_struct_features() {
    std::cout << "\n=== 结构体高级特性 ===" << std::endl;
    
    // 1. 位域结构体
    struct BitFieldStruct {
        unsigned int is_active : 1;
        unsigned int type : 3;
        unsigned int priority : 4;
        unsigned int : 0;  // 强制对齐到下一个整数
        unsigned int value : 16;
    } bits = {1, 5, 9, 1000};
    
    std::cout << "位域结构体:" << std::endl;
    std::cout << "  is_active: " << bits.is_active << std::endl;
    std::cout << "  type: " << bits.type << std::endl;
    std::cout << "  priority: " << bits.priority << std::endl;
    std::cout << "  value: " << bits.value << std::endl;
    std::cout << "  大小: " << sizeof(BitFieldStruct) << " 字节" << std::endl;
    
    // 2. 初始化列表
    AdvancedStruct s1{1001, "张三"};
    s1.print();
    
    AdvancedStruct s2 = {1002, "李四"};
    s2.print();
    
    // 3. 结构体中的结构体
    Company comp;
    comp.company_name = "ABC科技有限公司";
    comp.departments[0].name = "研发部";
    comp.departments[0].employee_count = 50;
    comp.departments[0].teams[0].leader = "王五";
    comp.departments[0].teams[0].member_count = 8;
    
    comp.foundation.established_year = 2000;
    std::cout << "\n公司: " << comp.company_name << std::endl;
    std::cout << "成立年份: " << comp.foundation.established_year << std::endl;
    
    // 4. 结构体数组
    AdvancedStruct employees[] = {
        {1001, "张三"},
        {1002, "李四"},
        {1003, "王五"}
    };
    
    std::cout << "\n员工列表:" << std::endl;
    for (const auto& emp : employees) {
        emp.print();
    }
}



#include <iostream>
#include <variant>  // C++17
#include <string>

// 匿名联合体
struct FlexibleData {
    enum Type { INT, FLOAT, STRING } type;
    
    union {
        int int_value;
        float float_value;
        char* string_value;  // 注意：需要手动管理内存！
    };
    
    FlexibleData(int v) : type(INT), int_value(v) {}
    FlexibleData(float v) : type(FLOAT), float_value(v) {}
    FlexibleData(const char* v) : type(STRING) {
        string_value = new char[strlen(v) + 1];
        strcpy(string_value, v);
    }
    
    ~FlexibleData() {
        if (type == STRING) {
            delete[] string_value;
        }
    }
    
    void print() const {
        switch(type) {
            case INT: std::cout << "int: " << int_value; break;
            case FLOAT: std::cout << "float: " << float_value; break;
            case STRING: std::cout << "string: " << string_value; break;
        }
    }
};

// 现代C++中的联合体（C++11起）
union ModernUnion {
    int i;
    float f;
    double d;
    
    // C++11: 联合体可以包含非平凡类型的成员
    std::string s;  // 必须提供构造函数/析构函数
    
    ModernUnion() : i(0) {}  // 默认构造
    ModernUnion(int val) : i(val) {}
    ModernUnion(float val) : f(val) {}
    ModernUnion(double val) : d(val) {}
    ModernUnion(const std::string& val) : s(val) {}
    
    ~ModernUnion() {
        // 必须手动调用非平凡类型的析构函数
        if (s.length() > 0) {
            s.~basic_string();
        }
    }
};

// 带标记的联合体（Tagged Union）
struct TaggedData {
    enum DataType { NONE, INTEGER, DECIMAL, TEXT } type;
    
    union {
        int integer;
        double decimal;
        char text[100];  // 固定大小，避免动态内存
    };
    
    TaggedData() : type(NONE) {
        integer = 0;
    }
    
    TaggedData(int val) : type(INTEGER), integer(val) {}
    TaggedData(double val) : type(DECIMAL), decimal(val) {}
    TaggedData(const char* val) : type(TEXT) {
        strncpy(text, val, sizeof(text) - 1);
        text[sizeof(text) - 1] = '\0';
    }
    
    ~TaggedData() {
        // 对于平凡类型，不需要特殊处理
    }
    
    void print() const {
        switch(type) {
            case NONE: std::cout << "None"; break;
            case INTEGER: std::cout << "Integer: " << integer; break;
            case DECIMAL: std::cout << "Decimal: " << decimal; break;
            case TEXT: std::cout << "Text: " << text; break;
        }
    }
};

// 变体类型（C++17替代方案）
using VariantData = std::variant<std::monostate, int, double, std::string>;

void demonstrate_union_features() {
    std::cout << "\n=== 联合体高级特性 ===" << std::endl;
    
    // 1. 匿名联合体示例
    {
        std::cout << "1. 匿名联合体:" << std::endl;
        struct {
            int type;
            union {
                int int_val;
                float float_val;
                char char_val;
            };
        } data = {0};
        
        data.type = 1;
        data.int_val = 42;
        std::cout << "  Type: " << data.type << ", Value: " << data.int_val << std::endl;
    }
    
    // 2. 带标记的联合体
    {
        std::cout << "\n2. 带标记的联合体:" << std::endl;
        TaggedData td1(100);
        TaggedData td2(3.14159);
        TaggedData td3("Hello World");
        
        std::cout << "  ";
        td1.print();
        std::cout << std::endl;
        
        std::cout << "  ";
        td2.print();
        std::cout << std::endl;
        
        std::cout << "  ";
        td3.print();
        std::cout << std::endl;
    }
    
    // 3. 现代C++联合体
    {
        std::cout << "\n3. 现代C++联合体（包含string）:" << std::endl;
        ModernUnion mu1(100);
        std::cout << "  mu1.i = " << mu1.i << std::endl;
        
        ModernUnion mu2(3.14f);
        std::cout << "  mu2.f = " << mu2.f << std::endl;
        
        ModernUnion mu3(std::string("Hello"));
        std::cout << "  mu3.s = " << mu3.s << std::endl;
        
        // 注意：必须正确管理生命周期
        mu3.s.~basic_string();
    }
    
    // 4. 类型双关（Type Punning）的合法用法
    {
        std::cout << "\n4. 类型双关（Type Punning）:" << std::endl;
        union FloatInt {
            float f;
            int i;
        } fi;
        
        fi.f = 3.14159f;
        std::cout << "  Float: " << fi.f << std::endl;
        std::cout << "  As int: 0x" << std::hex << fi.i << std::dec << std::endl;
        
        // 检查浮点数的IEEE 754表示
        int sign = (fi.i >> 31) & 0x1;
        int exponent = (fi.i >> 23) & 0xFF;
        int mantissa = fi.i & 0x7FFFFF;
        
        std::cout << "  IEEE 754 解析:" << std::endl;
        std::cout << "    符号位: " << sign << " (" << (sign ? "负" : "正") << ")" << std::endl;
        std::cout << "    指数: " << exponent << " (实际指数: " << (exponent - 127) << ")" << std::endl;
        std::cout << "    尾数: 0x" << std::hex << mantissa << std::dec << std::endl;
    }
    
    // 5. 网络字节序转换
    {
        std::cout << "\n5. 网络字节序转换:" << std::endl;
        union NetworkData {
            uint32_t value;
            uint8_t bytes[4];
        } data;
        
        data.value = 0x12345678;
        std::cout << "  原始值: 0x" << std::hex << data.value << std::dec << std::endl;
        std::cout << "  字节表示: ";
        for (int i = 0; i < 4; ++i) {
            std::cout << std::hex << (int)data.bytes[i] << " ";
        }
        std::cout << std::dec << std::endl;
        
        // 转换为网络字节序（大端）
        if (data.bytes[0] == 0x12) {  // 检查是否已是大端
            std::cout << "  已是大端序" << std::endl;
        } else {
            // 需要交换字节
            std::swap(data.bytes[0], data.bytes[3]);
            std::swap(data.bytes[1], data.bytes[2]);
            std::cout << "  转换为大端序: 0x";
            for (int i = 0; i < 4; ++i) {
                std::cout << std::hex << (int)data.bytes[i];
            }
            std::cout << std::dec << std::endl;
        }
    }
    
    // 6. std::variant（C++17更好的选择）
    {
        std::cout << "\n6. std::variant（C++17）:" << std::endl;
        VariantData v1 = 42;
        VariantData v2 = 3.14159;
        VariantData v3 = std::string("Hello");
        VariantData v4;  // std::monostate
        
        // 访问变体
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) {
                std::cout << "  int: " << arg << std::endl;
            } else if constexpr (std::is_same_v<T, double>) {
                std::cout << "  double: " << arg << std::endl;
            } else if constexpr (std::is_same_v<T, std::string>) {
                std::cout << "  string: " << arg << std::endl;
            } else {
                std::cout << "  empty" << std::endl;
            }
        }, v3);
        
        // 安全访问
        if (std::holds_alternative<int>(v1)) {
            std::cout << "  v1 包含 int: " << std::get<int>(v1) << std::endl;
        }
    }
}

#include<functional>
struct A{
    int x;
    union{
        int a;
        float b;
    };
    char buffer[2];
    union B{
        int c;
        float d;
    };   
    constexpr static const int f=10;    
     void print(){}
};

#include<variant>

int main(){
    
    demonstrate_basics();
    
    visualize_memory_layout();

    demonstrate_struct_features();
    std::cout<<sizeof(A)<<std::endl;
    A simple;
    simple.a=10;
    std::cout<<simple.a<<std::endl;
    simple.b=20.0;
    std::cout<<simple.b<<std::endl;
    std::cout<<simple.a<<std::endl;
    std::variant<int, float> v;
    
    v = 42;          // 存 int
    std::cout<<std::get<int>(v)<<std::endl;    
    v = 3.14f;       // 自动析构 int，存 float  ,所以不能同时访问两次int
    std::cout<<std::get<float>(v)<<std::endl;    
    return 0;
}
