#include "config_system.h"
#include "type_traits.h"
#include <iostream>

// 自定义配置结构体
struct LogConfig {
    std::string level;
    std::string file;
};

// 特化YAML转换
namespace YAML {
template<>
struct convert<LogConfig> {
    static bool decode(const Node& node, LogConfig& cfg) {
        cfg.level = node["level"].as<std::string>();
        cfg.file = node["file"].as<std::string>();
        return true;
    }
};
}

int main() {
    // 注册配置项
    auto log_cfg = sylar::ConfigManager::AddConfig<LogConfig>(
        "log", 
        {"info", "/var/log/app.log"}, 
        "Log system configuration"
    );

    // 注册变更回调
    log_cfg->addListener([](const LogConfig& old_val, const LogConfig& new_val) {
        std::cout << "Log config changed:\n"
                  << "  level: " << old_val.level << " -> " << new_val.level << "\n"
                  << "  file: " << old_val.file << " -> " << new_val.file << std::endl;
    });

    // 加载配置文件
    sylar::ConfigManager::LoadFromYaml("config.yaml");
    sylar::ConfigManager::StartWatch();

    // 测试动态获取配置
    while (true) {
        auto cfg = sylar::ConfigManager::Get<LogConfig>("log");
        std::cout << "Current log level: " << cfg.level << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return 0;
}