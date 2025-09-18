#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <shared_mutex>
#include <filesystem>
#include <atomic>
#include <yaml-cpp/yaml.h>

namespace sylar {

// 读写锁包装类（基于C++17 shared_mutex）
class RWSpinlock {
public:
    void lock() { m_mtx.lock(); }
    void unlock() { m_mtx.unlock(); }
    void lock_shared() { m_mtx.lock_shared(); }
    void unlock_shared() { m_mtx.unlock_shared(); }
private:
    std::shared_mutex m_mtx;
};

// 配置变量基类（类型擦除）
class ConfigVarBase {
public:
    using ptr = std::shared_ptr<ConfigVarBase>;
    virtual ~ConfigVarBase() = default;
    virtual void fromYaml(const YAML::Node& node) = 0;
};

// 泛型配置变量（核心模板类）
template<typename T>
class ConfigVar : public ConfigVarBase {
public:
    using UpdateCallback = std::function<void(const T& old_val, const T& new_val)>;

    ConfigVar(const std::string& name, const T& default_val, const std::string& desc = "")
        : m_name(name), m_val(default_val), m_desc(desc) {}

    // 线程安全获取值
    T getValue() const {
        std::shared_lock<RWSpinlock> lock(m_mutex);
        return m_val;
    }

    // 线程安全设置值（触发回调）
    void setValue(const T& v) {
        {
            std::unique_lock<RWSpinlock> lock(m_mutex);
            if (v == m_val) return;
            m_old_val = m_val;
            m_val = v;
        }
        if (m_cb) m_cb(m_old_val, m_val);
    }

    // 注册变更回调
    void addListener(UpdateCallback cb) {
        std::unique_lock<RWSpinlock> lock(m_mutex);
        m_cb = std::move(cb);
    }

    // 从YAML节点加载（需特化实现）
    void fromYaml(const YAML::Node& node) override {
        setValue(node.as<T>());
    }

private:
    std::string m_name;
    T m_val;
    T m_old_val;
    std::string m_desc;
    UpdateCallback m_cb;
    mutable RWSpinlock m_mutex;
};

// 配置管理器（单例模式）
class ConfigManager {
public:
    template<typename T>
    static std::shared_ptr<ConfigVar<T>> AddConfig(
        const std::string& name, 
        const T& default_val,
        const std::string& desc = "") {
        
        auto var = std::make_shared<ConfigVar<T>>(name, default_val, desc);
        std::unique_lock<RWSpinlock> lock(GetMutex());
        GetConfigs()[name] = var;
        return var;
    }

    template<typename T>
    static T Get(const std::string& name, const T& default_val = T()) {
        auto var = GetConfig(name);
        if (var) {
            return std::static_pointer_cast<ConfigVar<T>>(var)->getValue();
        }
        return default_val;
    }

    // 加载YAML配置文件
    static void LoadFromYaml(const std::filesystem::path& path) {
        auto yaml = YAML::LoadFile(path.string());
        for (const auto& node : yaml) {
            auto name = node.first.as<std::string>();
            if (auto var = GetConfig(name)) {
                var->fromYaml(node.second);
            }
        }
    }

    // 启动文件热更新监控
    static void StartWatch() {
        static std::atomic<bool> running{true};
        std::thread([] {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (const auto& [path, last_mtime] : GetWatchedFiles()) {
                    auto new_mtime = std::filesystem::last_write_time(path);
                    if (new_mtime != last_mtime) {
                        LoadFromYaml(path);
                        GetWatchedFiles()[path] = new_mtime;
                    }v
                }
            }
        }).detach();
    }

private:
    static ConfigVarBase::ptr GetConfig(const std::string& name) {
        std::shared_lock<RWSpinlock> lock(GetMutex());
        auto it = GetConfigs().find(name);
        return it != GetConfigs().end() ? it->second : nullptr;
    }

    // 单例数据存储
    static std::unordered_map<std::string, ConfigVarBase::ptr>& GetConfigs() {
        static std::unordered_map<std::string, ConfigVarBase::ptr> configs;
        return configs;
    }

    static std::unordered_map<std::filesystem::path, std::filesystem::file_time_type>& GetWatchedFiles() {
        static std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> files;
        return files;
    }

    static RWSpinlock& GetMutex() {
        static RWSpinlock mtx;
        return mtx;
    }
};

} // namespace sylar