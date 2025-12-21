//实现一个简单的配置系统config.cpp------------>简单自己实现增加配置


//后续实现sylar的配置系统

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


