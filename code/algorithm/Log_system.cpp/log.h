#pragma once
#include <atomic>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <cstdarg>

namespace sylar {

class Spinlock {
public:
    void lock() { while(flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

class LogLevel {
public:
    enum Level { DEBUG = 1, INFO, WARN, ERROR, FATAL };
    static const char* ToString(Level level);
};

class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;
    LogEvent(const char* file, int32_t line, LogLevel::Level level);
    
    const char* getFile() const { return m_file; }
    int32_t getLine() const { return m_line; }
    LogLevel::Level getLevel() const { return m_level; }
    std::stringstream& getSS() { return m_ss; }

    void format(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

private:
    const char* m_file;
    int32_t m_line;
    LogLevel::Level m_level;
    std::stringstream m_ss;
};

class AsyncLogQueue {
public:
    using Buffer = std::vector<char>;
    using BufferPtr = std::shared_ptr<Buffer>;

    static AsyncLogQueue* GetInstance();
    void push(BufferPtr buf);
    BufferPtr popAll();

private:
    AsyncLogQueue() = default;
    std::atomic<BufferPtr> m_currentBuffer{nullptr};
    Spinlock m_mutex;
    std::vector<BufferPtr> m_buffersToWrite;
};

class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;
    explicit LogFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S} %p %m%n");
    std::string format(LogEvent::ptr event);

private:
    std::string m_pattern;
};

class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;
    virtual ~LogAppender() = default;
    virtual void log(LogEvent::ptr event) = 0;
    void setFormatter(LogFormatter::ptr formatter);
    LogFormatter::ptr getFormatter() const;

protected:
    LogFormatter::ptr m_formatter;
    mutable Spinlock m_mutex;
};

class StdoutLogAppender : public LogAppender {
public:
    static StdoutLogAppender* GetInstance();
    void log(LogEvent::ptr event) override;

private:
    StdoutLogAppender() = default;
};

class FileLogAppender : public LogAppender {
public:
    explicit FileLogAppender(const std::string& filename);
    static FileLogAppender* GetInstance();
    void log(LogEvent::ptr event) override;
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
    std::atomic<uint64_t> m_lastFlushTime{0};
};

class Logger {
public:
    using ptr = std::shared_ptr<Logger>;
    explicit Logger(const std::string& name = "root");
    
    void log(LogLevel::Level level, LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);

    LogLevel::Level getLevel() const { return m_level.load(); }
    void setLevel(LogLevel::Level level) { m_level = level; }

private:
    std::string m_name;
    std::atomic<LogLevel::Level> m_level{LogLevel::DEBUG};
    std::vector<LogAppender::ptr> m_appenders;
    mutable Spinlock m_mutex;
};

class LoggerManager {
public:
    static LoggerManager* GetInstance();
    Logger::ptr getLogger(const std::string& name);

private:
    std::unordered_map<std::string, Logger::ptr> m_loggers;
    Spinlock m_mutex;
};

// 日志宏定义
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(__FILE__, __LINE__, level))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

} // namespace sylar












class Spinlock {
public:
    void lock() { while(flag.test_and_set(std::memory_order_acquire)); }
    void unlock() { flag.clear(std::memory_order_release); }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

class LogLevel {
public:
    enum Level { DEBUG = 1, INFO, WARN, ERROR, FATAL };
};


class LogEvent {
public:
    using ptr = std::shared_ptr<LogEvent>;
    std::stringstream& getSS() { return m_ss; }
    void format(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

private:
    const char* m_file;
    int32_t m_line;
    LogLevel::Level m_level;
    std::stringstream m_ss;
};


class LogFormatter {
public:
    using ptr = std::shared_ptr<LogFormatter>;
    explicit LogFormatter(const std::string& pattern = "%d{%Y-%m-%d %H:%M:%S} %p %m%n");
    std::string format(LogEvent::ptr event);

private:
    std::string m_pattern;
};



class StdoutLogAppender : public LogAppender {
public:
    static StdoutLogAppender* GetInstance();
    void log(LogEvent::ptr event) override;

private:
    StdoutLogAppender() = default;
};


class FileLogAppender : public LogAppender {
public:
    void log(LogEvent::ptr event) override;
    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
    std::atomic<uint64_t> m_lastFlushTime{0};
};



class LogAppender {
public:
    using ptr = std::shared_ptr<LogAppender>;
    virtual ~LogAppender() = default;
    virtual void log(LogEvent::ptr event) = 0;

protected:
    LogFormatter::ptr m_formatter;
    mutable Spinlock m_mutex;
};

class Logger {
public:
    using ptr = std::shared_ptr<Logger>;
    explicit Logger(const std::string& name = "root");
    
    void log(LogLevel::Level level, LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);

private:
    std::string m_name;
    std::atomic<LogLevel::Level> m_level{LogLevel::DEBUG};
    std::vector<LogAppender::ptr> m_appenders;
};


class sylar::LoggerManager{
private:
    std::unordered_map<std::string, Logger::ptr> m_loggers;
    
};


class LogLevel{
    enum Level{DEBUG,INFO,WARN,ERROR,FATAL};
};

class LogFormatter{
    string m_pattern;
};

class LogEvent{

    string m_file;
    int32_t m_line;
    LogLevel::Level m_level;
};

class  StdoutLogAppender:public LogAppender{
    void log()=override;
};

class FileLogAppender:public LogAppender{
    void log()=override;
    string m_filename;
};

class LogAppender{
public:
    virtual ~LogAppender(){};
    virtual void log()=0;   
private:

};

class Logger{
    vector<LogAppender::ptr> m_appenders;
};


class sylar::LoggerManager{
    unordered_map<string, Logger::ptr> m_loggers;
};