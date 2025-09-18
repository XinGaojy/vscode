// sylar_log_complete.cpp
#include "log.h"
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <deque>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <zstd.h>
//#include <librdkafka/rdkafka.h>
//#include <nlohmann/json.hpp>
//#include <prometheus/exposer.h>
// #include <prometheus/registry.h>
// #include <prometheus/counter.h>
// #include <prometheus/gauge.h>
// #include <inih/INIReader.h>

using namespace prometheus;
using json = nlohmann::json;

namespace sylar {

// ==================== 自旋锁 ====================
class Spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock() { while(flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }
};

// ==================== 日志级别 ====================
class LogLevel {
public:
    enum Level { DEBUG = 1, INFO, WARN, ERROR, FATAL };
    static const char* ToString(Level level) {
        static const char* strs[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        return strs[level-1];
    }
};

// ==================== 日志事件 ====================
class Logger; // 前向声明
class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;
    LogEvent(const char* file, int32_t line, LogLevel::Level level, Logger* logger) 
        : m_file(file), m_line(line), m_level(level), m_logger(logger) {}
    
    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    LogLevel::Level getLevel() const { return m_level; }
    std::stringstream& getSS() { return m_ss; }
    Logger* getLogger() const { return m_logger; }

    void format(const char* fmt, ...) {
        va_list al;
        va_start(al, fmt);
        char* buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if(len != -1) {
            m_ss << std::string(buf, len);
            free(buf);
        }
        va_end(al);
    }

private:
    const char* m_file = nullptr;
    int32_t m_line = 0;
    LogLevel::Level m_level;
    Logger* m_logger;
    std::stringstream m_ss;
};

// ==================== 日志事件包装器 ====================
class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr event) : m_event(event) {}
    ~LogEventWrap() {
        if (m_event && m_event->getLogger()) {
            m_event->getLogger()->log(m_event->getLevel(), m_event);
        }
    }
    std::stringstream& getSS() { return m_event->getSS(); }

private:
    LogEvent::ptr m_event;
};

// ==================== 日志格式化 ====================
class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;
    explicit LogFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S} %p %m%n") 
        : m_pattern(pattern) {}

    std::string format(LogEvent::ptr event) {
        char buf[64];
        time_t now = time(nullptr);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        std::stringstream ss;
        ss << "[" << buf << "] "
           << "[" << LogLevel::ToString(event->getLevel()) << "] "
           << event->getSS().str();
        return ss.str();
    }

private:
    std::string m_pattern;
};

// ==================== 异步日志队列 ====================
class AsyncLogQueue {
public:
    struct Buffer {
        std::vector<char> data;
        bool compressed = false;
    };
    using BufferPtr = std::shared_ptr<Buffer>;

    static AsyncLogQueue* GetInstance() {
        static AsyncLogQueue instance;
        return &instance;
    }

    void push(const std::string& tenant, BufferPtr buf, bool enable_compress) {
        std::lock_guard<Spinlock> lock(m_mutex);
        auto& queue = m_tenant_queues[tenant];
        
        if (enable_compress && buf->data.size() > 1024) {
            buf->data = ZstdCompress(buf->data);
            buf->compressed = true;
        }
        queue.push_back(buf);
    }

    BufferPtr pop(const std::string& tenant) {
        std::lock_guard<Spinlock> lock(m_mutex);
        auto it = m_tenant_queues.find(tenant);
        if (it == m_tenant_queues.end() || it->second.empty()) {
            return nullptr;
        }
        auto buf = it->second.front();
        it->second.pop_front();
        return buf;
    }

    size_t size(const std::string& tenant) const {
        std::lock_guard<Spinlock> lock(m_mutex);
        auto it = m_tenant_queues.find(tenant);
        return it != m_tenant_queues.end() ? it->second.size() : 0;
    }

private:
    static std::string ZstdCompress(const std::string& data, int level = 3) {
        size_t bound = ZSTD_compressBound(data.size());
        std::string output(bound, '\0');
        size_t compressed_size = ZSTD_compress(
            output.data(), bound,
            data.data(), data.size(),
            level
        );
        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error("Zstd compression failed");
        }
        output.resize(compressed_size);
        return output;
    }

    std::unordered_map<std::string, std::deque<BufferPtr>> m_tenant_queues;
    mutable Spinlock m_mutex;
};

// ==================== 日志输出目标 ====================
class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;
    virtual ~LogAppender() = default;
    virtual void log(const std::string& tenant, LogEvent::ptr event) = 0;
    virtual std::string toYamlString() const = 0;

    void setFormatter(LogFormatter::ptr formatter) {
        std::lock_guard<Spinlock> lock(m_mutex);
        m_formatter = formatter;
    }

    LogFormatter::ptr getFormatter() const {
        std::lock_guard<Spinlock> lock(m_mutex);
        return m_formatter;
    }

protected:
    LogFormatter::ptr m_formatter;
    mutable Spinlock m_mutex;
};

// ==================== 控制台输出 ====================
class StdoutLogAppender : public LogAppender {
public:
    static StdoutLogAppender* GetInstance() {
        static StdoutLogAppender instance;
        return &instance;
    }

    void log(const std::string& tenant, LogEvent::ptr event) override {
        if (getFormatter()) {
            std::cout << getFormatter()->format(event) << std::endl;
        } else {
            std::cout << event->getSS().str() << std::endl;
        }
    }

    std::string toYamlString() const override {
        return "type: StdoutLogAppender";
    }

private:
    StdoutLogAppender() {
        m_formatter.reset(new LogFormatter());
    }
};

// ==================== 文件输出 ====================
class FileLogAppender : public LogAppender {
public:
    explicit FileLogAppender(const std::string& filename) 
        : m_filename(filename) {
        reopen();
        m_formatter.reset(new LogFormatter());
    }

    static FileLogAppender* GetInstance() {
        static FileLogAppender instance("sylar.log");
        return &instance;
    }

    void log(const std::string& tenant, LogEvent::ptr event) override {
        uint64_t now = time(nullptr);
        if (now - m_lastFlushTime > 3) {
            m_filestream.flush();
            m_lastFlushTime = now;
        }
        
        auto msg = getFormatter()->format(event);
        auto buf = std::make_shared<AsyncLogQueue::Buffer>();
        buf->data.assign(msg.begin(), msg.end());
        AsyncLogQueue::GetInstance()->push(tenant, buf, m_enable_compress);
    }

    bool reopen() {
        std::lock_guard<Spinlock> lock(m_mutex);
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios::app);
        return m_filestream.is_open();
    }

    std::string toYamlString() const override {
        return "type: FileLogAppender\nfile: " + m_filename;
    }

    void enableCompression(bool enable) { m_enable_compress = enable; }

private:
    std::string m_filename;
    std::ofstream m_filestream;
    std::atomic<uint64_t> m_lastFlushTime{0};
    bool m_enable_compress = true;
    mutable Spinlock m_mutex;
};

// ==================== Kafka输出 ====================
class KafkaLogAppender : public LogAppender {
public:
    KafkaLogAppender(const std::string& brokers, const std::string& topic)
        : m_topic(topic) {
        rd_kafka_conf_t* conf = rd_kafka_conf_new();
        rd_kafka_conf_set(conf, "bootstrap.servers", brokers.c_str(), nullptr, 0);
        
        m_producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, nullptr, 0);
        if (!m_producer) {
            throw std::runtime_error("Failed to create Kafka producer");
        }
    }

    void log(const std::string& tenant, LogEvent::ptr event) override {
        json j;
        j["timestamp"] = time(nullptr);
        j["level"] = LogLevel::ToString(event->getLevel());
        j["message"] = event->getSS().str();
        j["file"] = event->getFile();
        j["line"] = event->getLine();
        j["tenant"] = tenant;
        
        std::string msg = j.dump();
        rd_kafka_produce(
            rd_kafka_topic_new(m_producer, m_topic.c_str(), nullptr),
            RD_KAFKA_PARTITION_UA,
            RD_KAFKA_MSG_F_COPY,
            const_cast<char*>(msg.data()), msg.size(),
            nullptr, 0, nullptr
        );
        rd_kafka_poll(m_producer, 0);
    }

    std::string toYamlString() const override {
        return "type: KafkaLogAppender\ntopic: " + m_topic;
    }

    ~KafkaLogAppender() {
        rd_kafka_flush(m_producer, 10 * 1000);
        rd_kafka_destroy(m_producer);
    }

private:
    rd_kafka_t* m_producer;
    std::string m_topic;
};

// ==================== 日志采样 ====================
class LogSampler {
public:
    static bool ShouldLog(LogLevel::Level level, double sample_rate = 0.01) {
        if (level != LogLevel::DEBUG) return true;
        
        static std::atomic<int> counter{0};
        return (counter++ % static_cast<int>(1.0 / sample_rate) == 0);
    }
};

// ==================== 日志器 ====================
class Logger {
public:
    using ptr = std::shared_ptr<Logger>;
    explicit Logger(const std::string& name, const std::string& tenant = "default") 
        : m_name(name), m_tenant(tenant) {}

    void log(LogLevel::Level level, LogEvent::ptr event) {
        if (level < m_level.load(std::memory_order_relaxed)) return;
        
        std::lock_guard<Spinlock> lock(m_mutex);
        for (auto& appender : m_appenders) {
            appender->log(m_tenant, event);
        }
    }

    void addAppender(LogAppender::ptr appender) {
        std::lock_guard<Spinlock> lock(m_mutex);
        m_appenders.push_back(appender);
    }

    void delAppender(LogAppender::ptr appender) {
        std::lock_guard<Spinlock> lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    LogLevel::Level getLevel() const { return m_level.load(); }
    void setLevel(LogLevel::Level level) { m_level = level; }

    const std::string& getName() const { return m_name; }
    const std::string& getTenant() const { return m_tenant; }

private:
    std::string m_name;
    std::string m_tenant;
    std::atomic<LogLevel::Level> m_level{LogLevel::DEBUG};
    std::vector<LogAppender::ptr> m_appenders;
    mutable Spinlock m_mutex;
};

// ==================== 日志管理器 ====================
class LoggerManager {
public:
    static LoggerManager* GetInstance() {
        static LoggerManager instance;
        return &instance;
    }

    Logger::ptr getLogger(const std::string& name, const std::string& tenant = "default") {
        std::lock_guard<Spinlock> lock(m_mutex);
        std::string key = tenant + ":" + name;
        auto it = m_loggers.find(key);
        if (it != m_loggers.end()) {
            return it->second;
        }
        
        Logger::ptr logger(new Logger(name, tenant));
        m_loggers[key] = logger;
        return logger;
    }

private:
    std::unordered_map<std::string, Logger::ptr> m_loggers;
    mutable Spinlock m_mutex;
};

// ==================== Prometheus监控 ====================
class LogMonitor {
public:
    static LogMonitor* GetInstance(int port = 8080) {
        static LogMonitor instance(port);
        return &instance;
    }

    void IncCounter(LogLevel::Level level) {
        m_log_counter->Add({{"level", LogLevel::ToString(level)}}).Increment();
    }

    void SetQueueSize(const std::string& tenant, size_t size) {
        m_queue_gauge->Add({{"tenant", tenant}}).Set(size);
    }

private:
    LogMonitor(int port) {
        m_exposer = std::make_unique<Exposer>("0.0.0.0:" + std::to_string(port));
        m_registry = std::make_shared<Registry>();
        m_exposer->RegisterCollectable(m_registry);

        m_log_counter = &BuildCounter()
            .Name("log_messages_total")
            .Help("Total log messages")
            .Register(*m_registry)
            .Add({});

        m_queue_gauge = &BuildGauge()
            .Name("log_queue_size")
            .Help("Current log queue size")
            .Register(*m_registry)
            .Add({});
    }

    std::unique_ptr<Exposer> m_exposer;
    std::shared_ptr<Registry> m_registry;
    Counter* m_log_counter;
    Gauge* m_queue_gauge;
};

// ==================== 日志宏 ====================
#define SYLAR_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level && sylar::LogSampler::ShouldLog(level)) \
        sylar::LogEventWrap(std::make_shared<sylar::LogEvent>( \
            __FILE__, __LINE__, level, logger.get())).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

} // namespace sylar

// ==================== 工作线程实现 ====================
void log_worker(const std::string& tenant) {
    auto queue = sylar::AsyncLogQueue::GetInstance();
    auto file_appender = sylar::FileLogAppender::GetInstance();
    auto monitor = sylar::LogMonitor::GetInstance();

    while (true) {
        auto buf = queue->pop(tenant);
        if (buf) {
            try {
                std::string msg;
                if (buf->compressed) {
                    size_t decompressed_size = ZSTD_getFrameContentSize(
                        buf->data.data(), buf->data.size()
                    );
                    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
                        throw std::runtime_error("Invalid compressed data");
                    }
                    msg.resize(decompressed_size);
                    size_t actual_size = ZSTD_decompress(
                        msg.data(), decompressed_size,
                        buf->data.data(), buf->data.size()
                    );
                    if (ZSTD_isError(actual_size)) {
                        throw std::runtime_error("Decompression failed");
                    }
                    msg.resize(actual_size);
                } else {
                    msg.assign(buf->data.begin(), buf->data.end());
                }

                file_appender->reopen();
                std::ofstream out("sylar.log", std::ios::app);
                out << msg << std::endl;
                
                monitor->SetQueueSize(tenant, queue->size(tenant));
            } catch (const std::exception& e) {
                std::cerr << "Log worker error: " << e.what() << std::endl;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// ==================== 测试用例 ====================
void test_all_features() {
    // 初始化监控
    sylar::LogMonitor::GetInstance(9080);

    // 创建租户A的日志器
    auto logger_a = sylar::LoggerManager::GetInstance()->getLogger("app", "tenant_a");
    logger_a->addAppender(sylar::StdoutLogAppender::GetInstance());
    logger_a->addAppender(sylar::FileLogAppender::GetInstance());
    //logger_a->addAppender(std::make_shared<sylar::KafkaLogAppender>(
        "localhost:9092", "logs_tenant_a"));
    logger_a->setLevel(sylar::LogLevel::DEBUG);

    // 创建租户B的日志器
    auto logger_b = sylar::LoggerManager::GetInstance()->getLogger("app", "tenant_b");
    logger_b->addAppender(sylar::StdoutLogAppender::GetInstance());
    logger_b->setLevel(sylar::LogLevel::INFO);

    // 测试日志输出
    SYLAR_LOG_DEBUG(logger_a) << "Debug message (tenant A)";
    SYLAR_LOG_INFO(logger_a) << "User login (tenant A)";
    SYLAR_LOG_ERROR(logger_b) << "Database error (tenant B)";

    // 测试日志采样
    for (int i = 0; i < 1000; ++i) {
        SYLAR_LOG_DEBUG(logger_a) << "Sampled debug " << i; // 约1%会被输出
    }

    // 测试结构化日志
    auto event = std::make_shared<sylar::LogEvent>(
        __FILE__, __LINE__, sylar::LogLevel::INFO, logger_a.get());
    event->format("User %s login", "admin");
    logger_a->log(event->getLevel(), event);
}

int main() {
    // 启动租户A的日志线程
    std::thread worker_a(log_worker, "tenant_a");
    worker_a.detach();

    // 启动租户B的日志线程
    std::thread worker_b(log_worker, "tenant_b");
    worker_b.detach();

    // 运行测试
    test_all_features();

    // 等待日志写入完成
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}



