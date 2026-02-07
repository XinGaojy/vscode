#include <iostream>
#include <typeinfo>
using namespace std;
void c_style_casts() {
    std::cout << "=== C风格类型转换 ===" << std::endl;
    
    // 1. 隐式转换
    int i = 42;
    double d = i;  // 隐式int转double
    std::cout << "1. 隐式转换: int " << i << " -> double " << d << std::endl;
    
    // 2. 显式C风格转换
    double pi = 3.14159;
    int int_pi = (int)pi;  // C风格显式转换
    std::cout << "2. C风格显式转换: double " << pi << " -> int " << int_pi << std::endl;
    
    // 3. 函数式转换
    float f = float(pi);  // 函数式转换
    std::cout << "3. 函数式转换: double " << pi << " -> float " << f << std::endl;
    
    // 4. 指针转换
    int value = 100;
    int* ptr = &value;
    void* void_ptr = (void*)ptr;  // 指针转void*
    int* ptr2 = (int*)void_ptr;  // void*转回int*
    std::cout << "4. 指针转换: " << *ptr2 << std::endl;
    
    // 5. 常量转换
    const int ci = 200;
    int* mutable_ptr = (int*)&ci;  // 去除const（危险！）
    *mutable_ptr = 300;  // 未定义行为！
    std::cout<<ci<<std::endl;
    std::cout << "5. 常量转换（危险！）: " << *mutable_ptr << std::endl;
    
    // 6. 重新解释转换
    int num = 0x12345678;
    char* bytes = (char*)num;
    std::cout << "6. 重新解释转换: 0x" << std::hex << num 
              << " 的字节: ";
     
#if 0
    for (size_t j = 0; j < sizeof(num); ++j) {
        std::cout << std::hex << (int)(unsigned char)bytes[j] << " ";
    }
#endif

    std::cout << std::dec << std::endl;
}


#include <iostream>
#include <cstring>

void c_style_problems() {
    std::cout << "\n=== C风格转换的问题 ===" << std::endl;
    
    // 问题1: 隐式转换可能导致数据丢失
    {
        double large_value = 1e100;
        int small_int = large_value;  // 溢出！
        std::cout << "1. 隐式截断: " << large_value 
                  << " -> " << small_int << " (数据丢失)" << std::endl;
    }
    
    // 问题2: 指针转换危险
    {
        double d = 3.14159;
        // 危险：double* 转 int*
        int* bad_ptr = (int*)&d;
        std::cout << "2. 危险指针转换: double* -> int*" << std::endl;
        std::cout << "   原始double: " << d << std::endl;
        std::cout << "   转为int: " << *bad_ptr << " (无意义值)" << std::endl;
    }
    
    // 问题3: 常量性被丢弃
    {
        const int secret = 42;
        int* hack = (int*)&secret;  // 丢弃const
        *hack = 100;  // 未定义行为！
        std::cout << "3. 丢弃常量性: 修改const值" << std::endl;
        std::cout << "   hack: " << *hack << std::endl;
        std::cout << "   secret: " << secret << " (可能仍是42)" << std::endl;
    }
    
    // 问题4: 继承层次中的错误转换
    {
        class Base {
        public:
            virtual ~Base() {}
            int base_data = 10;
        };
        
        class Derived : public Base {
        public:
            int derived_data = 20;
        };
        
        Base* base_ptr = new Base();
        // 危险：将Base*转换为Derived*
        Derived* derived_ptr = (Derived*)base_ptr;
        std::cout << "4. 继承层次错误转换:" << std::endl;
        std::cout << "   derived_data: " << derived_ptr->derived_data 
                  << " (未定义行为)" << std::endl;
        std::cout<<derived_ptr->base_data<<std::endl;
        delete base_ptr;
    }
}


#include <iostream>
#include <string>
#include <memory>

void demonstrate_static_cast() {
    std::cout << "\n=== static_cast ===" << std::endl;
    std::cout << "用于相关类型之间的安全转换" << std::endl;
    
    // 1. 基本类型转换
    {
        double pi = 3.14159;
        int int_pi = static_cast<int>(pi);  // 浮点转整数
        double back = static_cast<double>(int_pi);  // 整数转浮点
        
        std::cout << "1. 基本类型转换:" << std::endl;
        std::cout << "   double " << pi << " -> int " << int_pi << std::endl;
        std::cout << "   int " << int_pi << " -> double " << back << std::endl;
    }
    
    // 2. 继承层次中的向上转换（安全）
    {
        class Animal {
        public:
            virtual ~Animal() {}
            virtual void speak() const { std::cout << "Animal sound" << std::endl; }
        };
        
        class Dog : public Animal {
        public:
            void speak() const override { std::cout << "Woof!" << std::endl; }
            void wagTail() const { std::cout << "Tail wagging" << std::endl; }
        };
        
        Dog dog;
        Animal* animal_ptr = &dog;  // 向上转换，隐式安全
        
        // 向下转换，需要dynamic_cast
        Dog* dog_ptr = static_cast<Dog*>(animal_ptr);  // 假设我们知道它是Dog
        std::cout << "\n2. 继承层次转换:" << std::endl;
        dog_ptr->speak();
        dog_ptr->wagTail();
    }
    
    // 3. 空指针转换
    {
        int* ptr = nullptr;
        void* void_ptr = static_cast<void*>(ptr);
        int* ptr2 = static_cast<int*>(void_ptr);
        
        std::cout << "\n3. 空指针转换:" << std::endl;
        std::cout << "   nullptr转换安全" << std::endl;
    }
    
    // 4. 相关类型指针转换
    {
        class Base {
        public:
            int x = 10;
        };
        
        class Derived : public Base {
        public:
            int y = 20;
        };
        
        Derived d;
        Base* b = &d;  // 向上转换
        
        // 相关类的指针转换
        Derived* d2 = static_cast<Derived*>(b);

        std::cout << "\n4. 相关类型指针转换:" << std::endl;
        std::cout << "   Base x: " << b->x << std::endl;
        std::cout << "   d2->Derived y: " << d2->y << std::endl;

    }
    
    // 5. 枚举转换
    {
        enum Color { RED, GREEN, BLUE };
        enum class TrafficLight : int { RED, YELLOW, GREEN };
        
        Color c = GREEN;
        int int_color = static_cast<int>(c);
        TrafficLight tl = static_cast<TrafficLight>(int_color);
        
        std::cout << "\n5. 枚举转换:" << std::endl;
        std::cout << "   Color -> int: " << int_color << std::endl;
        std::cout << "   int -> TrafficLight: " << static_cast<int>(tl) << std::endl;
    }
    
    // 6. 自定义转换操作符
    {
        class Meter {
        private:
            double value;
        public:
            Meter(double v) : value(v) {}
            operator double() const {  // 转换到double
                return value;
            }
            operator int() const {     // 转换到int
                return static_cast<int>(value);
            }
        };
        
        class Kilometer {
        private:
            double value;
        public:
            Kilometer(double v) : value(v) {}
            operator Meter() const {   // 转换到Meter
                return Meter(value * 1000);
            }
        };
        
        Meter m(1000);
        double d = static_cast<double>(m);
        int i = static_cast<int>(m);
        
        Kilometer km(1);
        Meter m2 = static_cast<Meter>(km);
        
        std::cout << "\n6. 自定义转换操作符:" << std::endl;
        std::cout << "   Meter(1000) -> double: " << d << std::endl;
        std::cout << "   Meter(1000) -> int: " << i << std::endl;
        std::cout << "   Kilometer(1) -> Meter: " << static_cast<double>(m2) << std::endl;
    }
    
    // 7. static_cast的限制
    {
        std::cout << "\n7. static_cast的限制:" << std::endl;
        
        const int ci = 100;
        const int* pi = static_cast<const int*>(&ci);  // 错误：不能去除const
        
        double d = 3.14;
//        int* bad = static_cast<int*>(&d);  // 错误：不相关类型
        double b=10;
        double* ptr = &b;
        double* dptr = static_cast<double*>(ptr);  // 错误：不相关类型
        
        std::cout << "   static_cast不能去除const限定符" << std::endl;
        std::cout << "   static_cast不能在无关类型间转换" << std::endl;
    }
}
#include <iostream>
#include <typeinfo>
#include <memory>

void demonstrate_dynamic_cast() {
    std::cout << "\n=== dynamic_cast ===" << std::endl;
    std::cout << "用于多态类型的运行时类型检查转换" << std::endl;
    
    // 1. 向下转换（downcast）
    {
        class Shape {
        public:
            virtual ~Shape() {}
            virtual void draw() const = 0;
        };
        
        class Circle : public Shape {
        public:
            void draw() const override {
                std::cout << "Drawing Circle" << std::endl;
            }
            void setRadius(double r) {
                radius = r;
                std::cout << "Radius set to: " << r << std::endl;
            }
        private:
            double radius = 1.0;
        };
        
        class Rectangle : public Shape {
        public:
            void draw() const override {
                std::cout << "Drawing Rectangle" << std::endl;
            }
            void setDimensions(double w, double h) {
                width = w; height = h;
                std::cout << "Dimensions: " << w << "x" << h << std::endl;
            }
        private:
            double width = 1.0, height = 1.0;
        };
        
        Shape* shapes[] = { new Circle(), new Rectangle() };
        
        std::cout << "\n1. 向下转换示例:" << std::endl;
        for (int i = 0; i < 2; ++i) {
            // 尝试转换为Circle
            Circle* circle = dynamic_cast<Circle*>(shapes[i]);
            if (circle) {
                std::cout << "   Shape " << i << " 是 Circle" << std::endl;
                circle->setRadius(2.5);
            } else {
                std::cout << "   Shape " << i << " 不是 Circle" << std::endl;
            }
            
            // 尝试转换为Rectangle
            Rectangle* rect = dynamic_cast<Rectangle*>(shapes[i]);
            if (rect) {
                std::cout << "   Shape " << i << " 是 Rectangle" << std::endl;
                rect->setDimensions(3.0, 4.0);
            } else {
                std::cout << "   Shape " << i << " 不是 Rectangle" << std::endl;
            }
            
            delete shapes[i];
        }
    }
    
    // 2. 交叉转换（crosscast）
    {
        class Base {
        public:
            virtual ~Base() {}
        };
        
        class InterfaceA {
        public:
            virtual ~InterfaceA() {}
            virtual void methodA() = 0;
        };
        
        class InterfaceB {
        public:
            virtual ~InterfaceB() {}
            virtual void methodB() = 0;
        };
        
        class Derived : public Base, public InterfaceA, public InterfaceB {
        public:
            void methodA() override {
                std::cout << "   InterfaceA::methodA() called" << std::endl;
            }
            void methodB() override {
                std::cout << "   InterfaceB::methodB() called" << std::endl;
            }
        };
        
        std::cout << "\n2. 交叉转换示例:" << std::endl;
        Derived* derived = new Derived();
        Base* base = derived;
        
        // 从Base*到InterfaceA*（需要dynamic_cast）
        InterfaceA* if_a = dynamic_cast<InterfaceA*>(base);
        if (if_a) {
            std::cout << "   Base* -> InterfaceA* 成功" << std::endl;
            if_a->methodA();
        }
        
        // 从InterfaceA*到InterfaceB*
        InterfaceB* if_b = dynamic_cast<InterfaceB*>(if_a);
        if (if_b) {
            std::cout << "   InterfaceA* -> InterfaceB* 成功" << std::endl;
            if_b->methodB();
        }
        
        delete derived;
    }
    
    // 3. 引用转换
    {
        class Animal {
        public:
            virtual ~Animal() {}
        };
        
        class Dog : public Animal {
        public:
            void bark() { std::cout << "   Woof!" << std::endl; }
        };
        
        class Cat : public Animal {
        public:
            void meow() { std::cout << "   Meow!" << std::endl; }
        };
        
        std::cout << "\n3. 引用转换（会抛出异常）:" << std::endl;
        
        Dog dog;
        Animal& animal_ref = dog;
        
        try {
            Dog& dog_ref = dynamic_cast<Dog&>(animal_ref);
            dog_ref.bark();
            
            // 尝试错误转换
            Cat& cat_ref = dynamic_cast<Cat&>(animal_ref);  // 抛出std::bad_cast
        } catch (const std::bad_cast& e) {
            std::cout << "   转换失败: " << e.what() << std::endl;
        }
    }
    
    // 4. 智能指针转换
    {
        class Base {
        public:
            virtual ~Base() {}
            virtual void print() const { std::cout << "   Base" << std::endl; }
        };
        
        class Derived : public Base {
        public:
            void print() const override { std::cout << "   Derived" << std::endl; }
            void extra() const { std::cout <<   "   Extra method" << std::endl; }
        };
        
        std::cout << "\n4. 智能指针转换:" << std::endl;
        
        std::shared_ptr<Base> base_ptr = std::make_shared<Derived>();
        base_ptr->print();
        
        // 转换为派生类智能指针
        std::shared_ptr<Derived> derived_ptr = 
            std::dynamic_pointer_cast<Derived>(base_ptr);
        
        if (derived_ptr) {
            std::cout << "   转换成功" << std::endl;
            derived_ptr->extra();
            derived_ptr->print();
        } else {
            std::cout << "   转换失败" << std::endl;
        }
        
        // 错误转换示例
        std::shared_ptr<Base> another_base = std::make_shared<Base>();
        std::shared_ptr<Derived> bad_conversion = 
            std::dynamic_pointer_cast<Derived>(another_base);
        
        if (!bad_conversion) {
            std::cout << "   错误转换返回nullptr" << std::endl;
        }
    }
#if 0
    // 5. 性能考虑
    {
        std::cout << "\n5. dynamic_cast性能考虑:" << std::endl;
        std::cout << "   - 需要RTTI（运行时类型信息）支持" << std::endl;
        std::cout << "   - 比static_cast慢" << std::endl;
        std::cout << "   - 只适用于多态类型（有虚函数）" << std::endl;
        std::cout << "   - 失败时返回nullptr（指针）或抛出bad_cast（引用）" << std::endl;
    }
    
    // 6. 替代dynamic_cast的设计模式
    {
        std::cout << "\n6. dynamic_cast的替代方案:" << std::endl;
        
        // Visitor模式
        class Visitor;
        
        class Element {
        public:
            virtual ~Element() {}
            virtual void accept(Visitor& v) = 0;
        };
        
        class ConcreteElementA : public Element {
        public:
            void accept(Visitor& v) override;
            void operationA() { std::cout << "   ConcreteElementA::operationA()" << std::endl; }
        };
        
        class ConcreteElementB : public Element {
        public:
            void accept(Visitor& v) override;
            void operationB() { std::cout << "   ConcreteElementB::operationB()" << std::endl; }
        };
        
        class Visitor {
        public:
            virtual void visit(ConcreteElementA&) = 0;
            virtual void visit(ConcreteElementB&) = 0;
        };
        
        void ConcreteElementA::accept(Visitor& v) { v.visit(*this); }
        void ConcreteElementB::accept(Visitor& v) { v.visit(*this); }
        
        class ConcreteVisitor : public Visitor {
        public:
            void visit(ConcreteElementA& a) override {
                std::cout << "   Visitor访问ConcreteElementA" << std::endl;
                a.operationA();
            }
            void visit(ConcreteElementB& b) override {
                std::cout << "   Visitor访问ConcreteElementB" << std::endl;
                b.operationB();
            }
        };
        
        ConcreteElementA a;
        ConcreteElementB b;
        ConcreteVisitor visitor;
        
        Element* elements[] = { &a, &b };
        for (auto elem : elements) {
            elem->accept(visitor);
        }
    }

#endif

}
#include <iostream>


#if 1
void demonstrate_const_cast() {
    std::cout << "\n=== const_cast ===" << std::endl;
    std::cout << "用于添加或移除const/volatile限定符" << std::endl;
    
    // 1. 移除const（合法情况）
    {
        std::cout << "\n1. 合法使用：修改非const对象的const引用" << std::endl;
        
        int value = 42;
        const int& const_ref = value;  // 创建const引用
        
        // 合法：原始对象不是const
        int& mutable_ref = const_cast<int&>(const_ref);
        mutable_ref = 100;
        
        std::cout << "   原始值: " << value << std::endl;
        std::cout << "   const引用: " << const_ref << std::endl;
        std::cout << "   修改后: " << mutable_ref << std::endl;
    }
    
    // 2. 移除const（非法情况 - 未定义行为）
    {
        std::cout << "\n2. 非法使用：修改真正的const对象" << std::endl;
        
        const int true_const = 42;
        
        // 危险：尝试修改真正的const对象
        int& hack = const_cast<int&>(true_const);
        hack = 100;  // 未定义行为！
        
        std::cout << "   true_const: " << true_const << " (可能还是42)" << std::endl;
        std::cout << "   hack: " << hack << " (可能是100)" << std::endl;
        std::cout << "   注意：这是未定义行为！" << std::endl;
    }
    
    // 3. 添加const（总是安全）
    {
        std::cout << "\n3. 添加const限定符（总是安全）" << std::endl;
        
        int value = 42;
        int* ptr = &value;
        
        // 添加const
        const int* const_ptr = const_cast<const int*>(ptr);
        
        std::cout << "   原始值: " << *ptr << std::endl;
        std::cout << "   const指针: " << *const_ptr << std::endl;
        
        // 可以修改原始对象
        *ptr = 100;
        std::cout << "   修改后: " << *const_ptr << std::endl;
    }

#if 0
    // 4. 调用遗留的非const API
    {
        std::cout << "\n4. 实际应用：调用遗留API" << std::endl;
        
        // 模拟遗留API
        void legacy_api(char* str) {
            // 假设这个API需要修改字符串
            for (int i = 0; str[i] != '\0'; ++i) {
                if (str[i] >= 'a' && str[i] <= 'z') {
                    str[i] -= 32;  // 转大写
                }
            }
        }
        
        const char* original = "hello world";
        
        // 需要创建副本
        char* buffer = new char[strlen(original) + 1];
        strcpy(buffer, original);
        
        // 调用遗留API
        legacy_api(buffer);
        
        std::cout << "   原始: " << original << std::endl;
        std::cout << "   修改后: " << buffer << std::endl;
        
        delete[] buffer;
    }


#endif

    // 5. 实现const和非const版本的方法
    {
        std::cout << "\n5. 实现const和非const版本的方法" << std::endl;
        
        class Buffer {
        private:
            char* data;
            size_t size;
            
        public:
            Buffer(const char* str) : size(strlen(str)) {
                data = new char[size + 1];
                strcpy(data, str);
            }
            
            ~Buffer() { delete[] data; }
            
            // 非const版本
            char& operator[](size_t index) {
                if (index >= size) throw std::out_of_range("Index out of range");
                return data[index];
            }
            
            // const版本，避免代码重复
            const char& operator[](size_t index) const {
                // 使用const_cast调用非const版本
                return const_cast<Buffer*>(this)->operator[](index);
            }
            
            const char* get() const { return data; }
        };
        
        Buffer buf("Hello");
        const Buffer& const_buf = buf;
        
        std::cout << "   修改前: " << buf.get() << std::endl;
        buf[0] = 'h';  // 调用非const版本
        std::cout << "   修改后: " << buf.get() << std::endl;
        
        std::cout << "   只读访问: " << const_buf[1] << std::endl;
    }
    
    // 6. volatile限定符
    {
        std::cout << "\n6. volatile限定符" << std::endl;
        
        volatile int sensor_value = 100;
        
        // 移除volatile
        int* normal_ptr = const_cast<int*>(&sensor_value);
        
        // 添加volatile
        volatile int* volatile_ptr = const_cast<volatile int*>(normal_ptr);
        
        std::cout << "   原始值: " << sensor_value << std::endl;
        *normal_ptr = 200;
        std::cout << "   修改后: " << *volatile_ptr << std::endl;
    }
    
    // 7. 安全使用建议
    {
        std::cout << "\n7. 安全使用const_cast的建议:" << std::endl;
        std::cout << "   a. 只在你知道原始对象不是const时移除const" << std::endl;
        std::cout << "   b. 添加const总是安全的" << std::endl;
        std::cout << "   c. 避免修改真正的const对象" << std::endl;
        std::cout << "   d. 考虑使用mutable成员变量替代" << std::endl;
        std::cout << "   e. 文档化使用const_cast的原因" << std::endl;
    }
}
#endif
#include <iostream>
#include <cstring>
#include <iomanip>

void demonstrate_reinterpret_cast() {
    std::cout << "\n=== reinterpret_cast ===" << std::endl;
    std::cout << "用于低级、不安全的类型重新解释" << std::endl;
    
    // 1. 指针类型转换
    {
        std::cout << "\n1. 指针类型转换:" << std::endl;
        
        int value = 0x12345678;
        int* int_ptr = &value;
        
        // 转换为其他指针类型
        char* char_ptr = reinterpret_cast<char*>(int_ptr);
        float* float_ptr = reinterpret_cast<float*>(int_ptr);
        
        std::cout << "   原始int: 0x" << std::hex << *int_ptr << std::dec << std::endl;
        std::cout << "   字节表示: ";
        for (size_t i = 0; i < sizeof(int); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << (int)(unsigned char)char_ptr[i] << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "   解释为float: " << *float_ptr << " (无意义)" << std::endl;
    }
    
    // 2. 指针和整数之间的转换
    {
        std::cout << "\n2. 指针和整数之间的转换:" << std::endl;
        
        int value = 42;
        int* ptr = &value;
        
        // 指针转整数
        uintptr_t int_value = reinterpret_cast<uintptr_t>(ptr);
        std::cout << "   指针值: " << ptr << std::endl;
        std::cout << "   整数表示: 0x" << std::hex << int_value << std::dec << std::endl;
        
        // 整数转指针
        int* ptr2 = reinterpret_cast<int*>(int_value);
        std::cout << "   转换回指针: " << ptr2 << std::endl;
        std::cout << "   值: " << *ptr2 << std::endl;
    }
    
#if 0
    // 3. 函数指针转换
    {
        std::cout << "\n3. 函数指针转换:" << std::endl;
        
        // 普通函数
        void normal_function() {
            std::cout << "   普通函数被调用" << std::endl;
        }
        
        // 转换为void(*)()
        void (*void_func)() = reinterpret_cast<void(*)()>(normal_function);
        
        // 调用
        void_func();
        
        // 注意：这种转换是平台相关的
    }

#endif


    // 4. 序列化和反序列化
    {
        std::cout << "\n4. 序列化/反序列化示例:" << std::endl;
        
        struct Packet {
            uint32_t id;
            uint16_t length;
            uint8_t data[10];
        };
        
        Packet packet = {12345, 10, {1,2,3,4,5,6,7,8,9,10}};
        
        // 序列化：结构体转字节数组
        uint8_t* buffer = reinterpret_cast<uint8_t*>(&packet);
        
        std::cout << "   序列化数据: ";
        for (size_t i = 0; i < sizeof(Packet); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << (int)buffer[i] << " ";
        }
        std::cout << std::dec << std::endl;
        
        // 反序列化：字节数组转结构体
        Packet* packet2 = reinterpret_cast<Packet*>(buffer);
        std::cout << "   反序列化ID: " << packet2->id << std::endl;
        std::cout << "   长度: " << packet2->length << std::endl;
    }
    
    // 5. 多继承中的指针调整
    {
        std::cout << "\n5. 多继承中的指针调整:" << std::endl;
        
        class Base1 {
        public:
            virtual ~Base1() {}
            int data1 = 100;
        };
        
        class Base2 {
        public:
            virtual ~Base2() {}
            int data2 = 200;
        };
        
        class Derived : public Base1, public Base2 {
        public:
            int data3 = 300;
        };
        
        Derived d;
        Derived* derived_ptr = &d;
        Base1* base1_ptr = derived_ptr;    // 隐式转换
        Base2* base2_ptr = derived_ptr;    // 隐式转换
        
        std::cout << "   derived_ptr: " << derived_ptr << std::endl;
        std::cout << "   base1_ptr: " << base1_ptr << std::endl;
        std::cout << "   base2_ptr: " << base2_ptr << std::endl;
        
        // 使用reinterpret_cast绕过调整
        Base2* bad_base2 = reinterpret_cast<Base2*>(derived_ptr);
        std::cout << "   reinterpret_cast base2_ptr: " << bad_base2 << std::endl;
        std::cout << "   注意：这会导致错误的指针值！" << std::endl;
    }


#if 0

    // 6. 网络编程中的应用
    {
        std::cout << "\n6. 网络编程示例:" << std::endl;
        
        // 模拟网络数据包
        struct EthernetHeader {
            uint8_t dest[6];
            uint8_t src[6];
            uint16_t type;
        };
        
        struct IPHeader {
            uint8_t version_ihl;
            uint8_t tos;
            uint16_t total_length;
            // ... 其他字段
        };
        
        // 接收到的数据
        uint8_t packet_data[1024];
        memset(packet_data, 0, sizeof(packet_data));
        
        // 填充示例数据
        EthernetHeader* eth = reinterpret_cast<EthernetHeader*>(packet_data);
        memcpy(eth->dest, "\x00\x11\x22\x33\x44\x55", 6);
        memcpy(eth->src, "\x66\x77\x88\x99\xAA\xBB", 6);
        eth->type = 0x0800;  // IPv4
        
        // 获取IP头
        IPHeader* ip = reinterpret_cast<IPHeader*>(packet_data + sizeof(EthernetHeader));
        ip->version_ihl = 0x45;
        ip->total_length = htons(100);
        
        std::cout << "   Ethernet类型: 0x" << std::hex << ntohs(eth->type) << std::dec << std::endl;
        std::cout << "   IP版本: " << ((ip->version_ihl >> 4) & 0x0F) << std::endl;
    }
    
    // 7. 危险和限制
    {
        std::cout << "\n7. reinterpret_cast的危险:" << std::endl;
        std::cout << "   a. 不进行类型检查" << std::endl;
        std::cout << "   b. 可能违反严格别名规则" << std::endl;
        std::cout << "   c. 平台相关" << std::endl;
        std::cout << "   d. 可能破坏多继承的指针调整" << std::endl;
        std::cout << "   e. 应作为最后的手段" << std::endl;
        
        // 严格别名规则违规示例
        int x = 42;
        float* fptr = reinterpret_cast<float*>(&x);
        *fptr = 3.14f;  // 违反严格别名规则！
        
        std::cout << "   违反严格别名规则 - 未定义行为！" << std::endl;
    }

#endif


}


int main(){
    
    c_style_casts();
    c_style_problems();
    demonstrate_static_cast();
    demonstrate_dynamic_cast();
    demonstrate_const_cast();

    demonstrate_reinterpret_cast();

cout<<"-----------------------------------"<<endl;
    // 示例1：去除const限定
    const int ci = 10;
    int* pi = const_cast<int*>(&ci);  // 去const
    *pi = 20;  // 未定义行为！实际内存可能被优化为只读
    cout<<ci<<endl;
    cout<<*pi<<endl;

    int i = 42;
    float& fr = reinterpret_cast<float&>(i);  // 别名违规
    return 0;
}
