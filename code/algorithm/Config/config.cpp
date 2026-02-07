//实现一个简单的配置系统config.cpp------------>简单自己实现增加配置


//后续实现sylar的配置系统


#if 1

#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <variant>

class Config {
private:
    using Val = std::variant<int, double, bool, std::string, std::shared_ptr<Config>>;
    std::map<std::string, Val> data;
    
public:
    // 通用set方法
    template<typename T>
    void set(const std::string& key, T value) {
        data[key] = value;
    }
    
    // 获取嵌套配置
    Config& at(const std::string& key) {
        auto it = data.find(key);
        if (it == data.end()) {
            auto sub = std::make_shared<Config>();
            data[key] = sub;
            return *sub;
        }
        
        if (auto ptr = std::get_if<std::shared_ptr<Config>>(&it->second)) {
            return **ptr;
        }
        
        throw std::runtime_error(key + " is not a config");
    }
    
    // 获取值
    template<typename T>
    T as(const std::string& key, T fallback = T{}) const {
        auto it = data.find(key);
        if (it == data.end()) return fallback;
        
        if (auto ptr = std::get_if<T>(&it->second)) {
            return *ptr;
        }
        
        return fallback;
    }
    
    // 输出
    void show(int depth = 0) const {
        std::string pad(depth * 2, ' ');
        for (const auto& [k, v] : data) {
            std::cout << pad << k << ": ";
            
            std::visit([&](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                
                if constexpr (std::is_same_v<T, int> || 
                             std::is_same_v<T, double>) {
                    std::cout << val;
                }
                else if constexpr (std::is_same_v<T, bool>) {
                    std::cout << (val ? "true" : "false");
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << "\"" << val << "\"";
                }
                else if constexpr (std::is_same_v<T, std::shared_ptr<Config>>) {
                    std::cout << "{\n";
                    val->show(depth + 1);
                    std::cout << pad << "}";
                }
            }, v);
            
            std::cout << std::endl;
        }
    }
};

int main() {

#if 1
    Config cfg;
    
    cfg.set("name", "App");
    cfg.set("ver", 1);
    
    // 链式设置嵌套
    cfg.at("db").set("host", "localhost");
    cfg.at("db").set("port", 3306);
    
    cfg.at("db").at("redis").set("port", 6379);
    cfg.at("db").at("redis").set("enabled", true);
    
    cfg.at("cache").set("size", 1024);
    
    // 展示配置
    cfg.show();
    
    // 获取值
    std::cout << "\nDB host: " << cfg.at("db").as<std::string>("host") << std::endl;
    std::cout << "Redis port: " << cfg.at("db").at("redis").as<int>("port") << std::endl;
   
/////////////////////////////////////////////////////////////////////////////////

#if 0

//**************************************************************************//

//结果输出

cache: {
  size: 1024
}
db: {
  host: "localhost"
  port: 3306
  redis: {
    enabled: true
    port: 6379
  }
}
name: "App"
ver: 1

DB host: localhost
Redis port: 6379
//****************************************************************************//    


#endif







//////////////////////////////////////////////////////////////////////////////////////


#endif


    return 0;
}


#endif















#if 0
//使用模板元编程实现编译时的配置器
#include <iostream>
#include <array>
#include <string>
#include <vector>
#include <chrono>

// 编译时配置系统
template<typename Config>
class ConfigSystem {
    static constexpr auto config = Config::generate();
    
public:
    template<auto Key>
    static constexpr auto get() {
        return std::get<Config::template find<Key>()>(config);
    }
    
    static void print() {
        std::cout << "配置系统:\n";
        Config::print_config();
    }
};

// 数据库配置示例
struct DatabaseConfig {
    static constexpr std::string_view name = "database";
    
    struct Keys {
        static constexpr std::string_view host = "host";
        static constexpr std::string_view port = "port";
        static constexpr std::string_view user = "user";
        static constexpr std::string_view password = "password";
        static constexpr std::string_view database = "database";
    };
    
    static constexpr auto generate() {
        return std::make_tuple(
            std::make_pair(Keys::host, "localhost"),
            std::make_pair(Keys::port, 5432),
            std::make_pair(Keys::user, "admin"),
            std::make_pair(Keys::password, "secret123"),
            std::make_pair(Keys::database, "myapp")
        );
    }
    
    template<auto Key>
    static constexpr size_t find() {
        constexpr auto config = generate();
        return find_impl<Key>(config, std::make_index_sequence<std::tuple_size_v<decltype(config)>>{});
    }
    
    template<auto Key, typename Tuple, size_t... Is>
    static constexpr size_t find_impl(const Tuple&, std::index_sequence<Is...>) {
        size_t index = 0;
        (((std::get<Is>(generate()).first == Key ? (index = Is, false) : false) || ...), true);
        return index;
    }
    
    static void print_config() {
        constexpr auto config = generate();
        std::apply([](auto&&... items) {
            ((std::cout << "  " << items.first << " = " << items.second << "\n"), ...);
        }, config);
    }
};

// 编译时路由系统
template<typename... Routes>
class Router {
    static constexpr std::array routes = {Routes::template match<>()...};
    
public:
    template<auto Path>
    static constexpr auto route() {
        for (const auto& route : routes) {
            if (route.first == Path) {
                return route.second;
            }
        }
        return []() { return "404 Not Found"; };
    }
    
    static void print_routes() {
        std::cout << "可用路由:\n";
        for (const auto& route : routes) {
            std::cout << "  " << route.first << "\n";
        }
    }
};

template<auto Path, auto Handler>
struct Route {
    template<typename = void>
    static constexpr auto match() {
        return std::make_pair(Path, []() { return Handler(); });
    }
};

// 编译时工厂模式
template<typename... Types>
class Factory {
    static constexpr std::array type_names = {TypeInfo<Types>::name...};
    
public:
    template<typename T>
    static T create() {
        std::cout << "创建: " << TypeInfo<T>::name << "\n";
        return T{};
    }
    
    template<auto TypeName>
    static auto create_by_name() {
        constexpr size_t index = []() {
            for (size_t i = 0; i < type_names.size(); ++i) {
                if (type_names[i] == TypeName) {
                    return i;
                }
            }
            return type_names.size();
        }();
        
        static_assert(index < type_names.size(), "未知类型");
        return create_by_index<index>();
    }
    
private:
    template<size_t Index>
    static auto create_by_index() {
        using T = std::tuple_element_t<Index, std::tuple<Types...>>;
        return create<T>();
    }
};

// 性能监控装饰器
template<typename T>
class Monitored {
    T instance;
    
public:
    template<typename... Args>
    Monitored(Args&&... args) : instance(std::forward<Args>(args)...) {}
    
    template<auto Method, typename... Args>
    auto call(Args&&... args) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = (instance.*Method)(std::forward<Args>(args)...);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "方法调用耗时: " << duration.count() << " 微秒\n";
        
        return result;
    }
};

void demonstrate_comprehensive_example() {
    std::cout << "\n=== 综合示例 ===\n";
    
    // 1. 配置系统
    std::cout << "\n1. 编译时配置系统:\n";
    using DBConfig = ConfigSystem<DatabaseConfig>;
    DBConfig::print();
    
    std::cout << "\n获取配置值:\n";
    std::cout << "host: " << DBConfig::get<DatabaseConfig::Keys::host>() << "\n";
    std::cout << "port: " << DBConfig::get<DatabaseConfig::Keys::port>() << "\n";
    
    // 2. 路由系统
    std::cout << "\n2. 编译时路由系统:\n";
    
    auto home_handler = []() { return "Home Page"; };
    auto about_handler = []() { return "About Page"; };
    auto contact_handler = []() { return "Contact Page"; };
    
    using AppRouter = Router<
        Route<"/", home_handler>,
        Route<"/about", about_handler>,
        Route<"/contact", contact_handler>
    >;
    
    AppRouter::print_routes();
    
    std::cout << "\n路由测试:\n";
    std::cout << "GET / -> " << AppRouter::route<"/">()() << "\n";
    std::cout << "GET /about -> " << AppRouter::route<"/about">()() << "\n";
    std::cout << "GET /unknown -> " << AppRouter::route<"/unknown">()() << "\n";
    
    // 3. 工厂模式
    std::cout << "\n3. 编译时工厂:\n";
    
    struct WidgetA { void use() { std::cout << "使用WidgetA\n"; } };
    struct WidgetB { void use() { std::cout << "使用WidgetB\n"; } };
    struct WidgetC { void use() { std::cout << "使用WidgetC\n"; } };
    
    template<> struct TypeInfo<WidgetA> { static constexpr std::string_view name = "WidgetA"; };
    template<> struct TypeInfo<WidgetB> { static constexpr std::string_view name = "WidgetB"; };
    template<> struct TypeInfo<WidgetC> { static constexpr std::string_view name = "WidgetC"; };
    
    using WidgetFactory = Factory<WidgetA, WidgetB, WidgetC>;
    
    auto widget1 = WidgetFactory::create_by_name<"WidgetA">();
    auto widget2 = WidgetFactory::create_by_name<"WidgetB">();
    
    widget1.use();
    widget2.use();
    
    // 4. 性能监控
    std::cout << "\n4. 编译时性能监控:\n";
    
    struct ExpensiveOperation {
        int compute(int n) {
            // 模拟耗时操作
            int result = 0;
            for (int i = 0; i < n * 1000; ++i) {
                result += i;
            }
            return result;
        }
        
        void process(const std::string& data) {
            // 模拟处理
            for (char c : data) {
                volatile char temp = c;  // 防止优化
            }
        }
    };
    
    Monitored<ExpensiveOperation> monitored_op;
    
    std::cout << "计算操作:\n";
    int result = monitored_op.call<&ExpensiveOperation::compute>(1000);
    std::cout << "结果: "<<std::endl;
}



int main(){

    demonstrate_comprehensive_example();
    return 0;
}
#endif


