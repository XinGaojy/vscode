#if 0

#if 1
//隐式实例化
#include <iostream>
#include <vector>
#include<memory.h>
// 类模板
template<typename T>
class MyClass {
public:
    MyClass(){std::cout << "init_myclass" << std::endl;}
    virtual void virfunc(){
        std::cout<<"hello virtual func()"<<std::endl;
    }
    void used_function() {
        std::cout << "MyClass::used_function() instantiated for T = " 
                  << typeid(T).name() << "\n";
    }
    
    void unused_function() {
        std::cout << "MyClass::unused_function() for T = " 
                  << typeid(T).name() << "\n";
        // 这里可能依赖于T的一些操作
        T unused_var = T{};
        (void)unused_var;  // 防止警告
    }
    
    // 嵌套类型
    using value_type = T;
    
    // 静态成员
    static int static_member;
    
    // 内部类
    class InnerClass {
    public:
        void inner_method() {
            std::cout << "InnerClass::inner_method()\n";
        }
    };
};

// 静态成员定义
template<typename T>
int MyClass<T>::static_member = 0;

// 函数模板
template<typename T>
void process_value(T value) {
    std::cout << "process_value called with T = " << typeid(T).name() 
              << ", value = " << value << "\n";
}

// 带默认参数的模板
template<typename T = int>
class DefaultTemplate {
    T value;
public:
    DefaultTemplate(T v = T{}) : value(v) {}
    T get() const { return value; }
};

void demonstrate_implicit_instantiation() {
    std::cout << "\n=== 隐式实例化 ===\n";
    
    // 1. 创建对象时实例化
    MyClass<int> obj1;           // 实例化 MyClass<int>
    obj1.used_function();        // 实例化 used_function<int>
    // obj1.unused_function();   // 没有调用，不会实例化
    
    // 2. 使用静态成员时实例化
    MyClass<int>::static_member = 42;  // 实例化静态成员
    
    // 3. 创建嵌套类型对象时实例化
    MyClass<double>::InnerClass inner;  // 实例化 InnerClass
    // inner.inner_method();  // 没有调用，inner_method不会实例化
    
    // 4. 使用类型别名
    using IntValue = MyClass<int>::value_type;  // 不实例化任何代码
    IntValue x = 10;  // 只是类型别名，不实例化模板
    
    // 5. 函数调用时实例化
    process_value(10);           // 实例化 process_value<int>
    process_value(3.14);         // 实例化 process_value<double>
    
    // 6. 默认参数
    DefaultTemplate<> defaultObj;  // 使用默认参数int
    std::cout << "defaultObj.get() = " << defaultObj.get() << "\n";
    
    DefaultTemplate<double> doubleObj(3.14);
    std::cout << "doubleObj.get() = " << doubleObj.get() << "\n";
    
    // 7. 在表达式中使用
    auto lambda = []() {
        MyClass<float> temp;  // 实例化 MyClass<float>
        temp.used_function();
        temp.virfunc();
        return 0;
    }();

    //在lambda表达式中通过在最后的结尾使用()来实现直接运行,现场调用
    // lambda还没有被调用，所以MyClass<float>还没有实例化,不会调用构造函数
    // lambda();  // 调用时会实例化

    []() {
        MyClass<float> temp;  // 实例化 MyClass<float>
        temp.used_function();
        return 0;
    }();
    

    std::cout << "\n延迟实例化示例:\n";
    
    // 模板的延迟实例化
    MyClass<std::string> strObj;
    // 此时只有构造函数被实例化
    strObj.used_function();  // 实例化 used_function<std::string>
    
    // 注意：虚函数是特殊的
    // 如果模板中有虚函数，它会在类实例化时立即被实例化
    // 因为虚表需要包含所有虚函数的地址
}
#endif



#if 1
//隐式实例化

// 大型类模板示例
template<typename T>
class ComplexTemplate {
private:
    std::vector<T> data;
    
public:
    ComplexTemplate() = default;
    explicit ComplexTemplate(size_t size) : data(size) {}
    
    // 许多成员函数
    void push_back(const T& value) { 
        std::cout << "push_back for T = " << typeid(T).name() << "\n";
        data.push_back(value); 
    }
    
    T pop_back() { 
        std::cout << "pop_back for T = " << typeid(T).name() << "\n";
        T value = data.back();
        data.pop_back();
        return value;
    }
    
    T& operator[](size_t index) { 
        std::cout << "operator[] for T = " << typeid(T).name() << "\n";
        return data[index]; 
    }
    
    const T& operator[](size_t index) const { 
        std::cout << "const operator[] for T = " << typeid(T).name() << "\n";
        return data[index]; 
    }
    
    size_t size() const { 
        std::cout << "size() for T = " << typeid(T).name() << "\n";
        return data.size(); 
    }
    
    // 更多成员函数...
    void clear() { data.clear(); }
    bool empty() const { return data.empty(); }
    void reserve(size_t n) { data.reserve(n); }
    
    // 迭代器
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};

// 显式实例化声明 (在头文件中)
extern template class ComplexTemplate<int>;
extern template class ComplexTemplate<double>;
extern template class ComplexTemplate<std::string>;
// 显式实例化对应的函数
#if 0
extern template void ComplexTemplate<int>::push_back(const int&);
extern template int ComplexTemplate<int>::pop_back();
extern template int& ComplexTemplate<int>::operator[](size_t);
extern template const int& ComplexTemplate<int>::operator[](size_t) const;
extern template ComplexTemplate<int>::push_back(int const& );
extern template ComplexTemplate<int>::size()const;
extern template ComplexTemplate<int>::operator[](unsigned long);
extern template ComplexTemplate<double>::push_back(double const&);


#endif


// 在不同的编译单元中，我们需要提供显式实例化定义
// 例如在 .cpp 文件中：
// template class ComplexTemplate<int>;
// template class ComplexTemplate<double>;
// template class ComplexTemplate<std::string>;

void demonstrate_explicit_instantiation() {
    std::cout << "=== 显式实例化 ===\n";
    
    // 使用显式实例化的类型
    ComplexTemplate<int> intContainer;
    intContainer.push_back(1);
    intContainer.push_back(2);
    std::cout << "intContainer size: " << intContainer.size() << "\n";
    std::cout << "intContainer[0]: " << intContainer[0] << "\n";
    
    ComplexTemplate<double> doubleContainer;
    doubleContainer.push_back(3.14);
    
    ComplexTemplate<std::string> stringContainer;
    stringContainer.push_back("hello");
    
    // 如果没有显式实例化，每个使用的地方都会实例化模板
    // 有了显式实例化，链接器会使用预先实例化的版本
}



#endif


int main(){
    //隐式实例化
    demonstrate_implicit_instantiation();
    //显示实例化
    demonstrate_explicit_instantiation();



    return 0;
}




#endif


#include <iostream>
#include <vector>
#include <typeinfo>
#include <string>

/* ---------- 隐式实例化部分 ---------- */
template<typename T>
class MyClass {
public:
    MyClass() { std::cout << "init_myclass<" << typeid(T).name() << ">\n"; }

    virtual void virfunc() { std::cout << "hello virtual func()\n"; }

    void used_function() {
        std::cout << "MyClass::used_function() instantiated for T = "
                  << typeid(T).name() << "\n";
    }
    void unused_function() {           // 未调用，不会实例化
        T unused_var{};
        (void)unused_var;
    }

    using value_type = T;
    static int static_member;

    class InnerClass {
    public:
        void inner_method() { std::cout << "InnerClass::inner_method()\n"; }
    };
};

template<typename T>
int MyClass<T>::static_member = 0;

template<typename T>
void process_value(T value) {
    std::cout << "process_value called with T = " << typeid(T).name()
              << ", value = " << value << "\n";
}

template<typename T = int>
class DefaultTemplate {
    T value{};
public:
    explicit DefaultTemplate(T v = T{}) : value(v) {}
    T get() const { return value; }
};

void demonstrate_implicit_instantiation() {
    std::cout << "=== 隐式实例化 ===\n";

    MyClass<int> obj1;                       // 实例化类 + 构造函数
    obj1.used_function();                    // 实例化 used_function
    MyClass<int>::static_member = 42;        // 实例化静态成员变量

    MyClass<double>::InnerClass inner;       // 实例化 InnerClass
    inner.inner_method();                    // 实例化 inner_method

    using IntValue = MyClass<int>::value_type;
    IntValue x = 10;

    process_value(10);                       // 实例化 process_value<int>
    process_value(3.14);                     // 实例化 process_value<double>

    DefaultTemplate<> defaultObj;            // 默认模板实参 = int
    std::cout << "defaultObj.get() = " << defaultObj.get() << "\n";
    DefaultTemplate<double> doubleObj(3.14);
    std::cout << "doubleObj.get() = " << doubleObj.get() << "\n";

    /* 就地调用 lambda，现场实例化 MyClass<float> */
    []() {
        MyClass<float> temp;
        temp.used_function();
        temp.virfunc();
        return 0;
    }();
}

/* ---------- 显式实例化部分 ---------- */
template<typename T>
class ComplexTemplate {
    std::vector<T> data;
public:
    ComplexTemplate() = default;
    explicit ComplexTemplate(size_t sz) : data(sz) {}

    void push_back(const T& v) {
        std::cout << "push_back T=" << typeid(T).name() << "\n";
        data.push_back(v);
    }
    T pop_back() {
        std::cout << "pop_back T=" << typeid(T).name() << "\n";
        T val = data.back();
        data.pop_back();
        return val;
    }
    T& operator[](size_t i)             { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }
    size_t size() const { return data.size(); }
};

/* ---- 显式实例化声明（告诉编译器“别处已有定义”） ---- */
extern template class ComplexTemplate<int>;
extern template class ComplexTemplate<double>;
extern template class ComplexTemplate<std::string>;

/* ---- 在头文件里直接给出定义，演示也能链接通过 ---- */
template class ComplexTemplate<int>;
template class ComplexTemplate<double>;
template class ComplexTemplate<std::string>;

void demonstrate_explicit_instantiation() {
    std::cout << "\n=== 显式实例化 ===\n";

    ComplexTemplate<int> intContainer;
    intContainer.push_back(1);
    intContainer.push_back(2);
    std::cout << "intContainer size: " << intContainer.size() << "\n";
    std::cout << "intContainer[0]: " << intContainer[0] << "\n";

    ComplexTemplate<double> doubleContainer;
    doubleContainer.push_back(3.14);

    ComplexTemplate<std::string> stringContainer;
    stringContainer.push_back("hello");
}

/* ---------- main ---------- */
int main() {
    demonstrate_implicit_instantiation();
    demonstrate_explicit_instantiation();
    return 0;
}
