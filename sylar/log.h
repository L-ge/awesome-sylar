/**
 * @filename    log.h
 * @brief   日志模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <stdarg.h>
#include <iostream>

#include "mutex.h"
#include "util.h"
#include "singleton.h"
#include "thread.h"

/**
 * @brief   使用流式方式写日志
 */
#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

/**
 * @brief   使用格式化方式写日志
 */
#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, sylar::GetThreadId(),\
                sylar::GetFiberId(), time(0), sylar::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...)  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)


/**
 * @brief   获取主日志器
 */
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()


/**
 * @brief   获取name的日志器
 */
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar
{
class Logger;

/**
 * @brief   日志级别封装类
 */
class LogLevel
{
public:
    enum Level
    {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };
    
    static const char* ToString(LogLevel::Level level);

    static LogLevel::Level FromString(const std::string& str);
};

/**
 * @brief  日志事件类 
 */
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> ptr;

    LogEvent(std::shared_ptr<Logger> logger, 
             LogLevel::Level level,
             const char* file,
             int32_t line,
             uint32_t elapse,
             uint32_t threadId,
             uint32_t fiberId,
             uint64_t time,
             const std::string& threadName);

    const char* getFile() const { return m_file; }

    int32_t getLine() const { return m_line; } 

    uint32_t getElapse() const { return m_elapse; }

    uint32_t getThreadId() const { return m_threadId; }

    uint32_t getFiberId() const { return m_fiberId; }

    uint64_t getTime() const { return m_time; }

    const std::string& getThreadName() const { return m_threadName; }

    std::string getContent() const { return m_ss.str(); }

    std::shared_ptr<Logger> getLogger() const { return m_logger; }

    LogLevel::Level getLevel() const { return m_level; }

    std::stringstream& getSS() { return m_ss; }

    void format(const char* fmt, ...);

    void format(const char* fmt, va_list al);

private:
    /// 日志器
    std::shared_ptr<Logger> m_logger;
    /// 日志等级
    LogLevel::Level m_level;
    /// 文件名
    const char* m_file = nullptr;
    /// 行号
    int32_t m_line = 0;
    /// 程序运行的毫秒数
    uint32_t m_elapse = 0;
    /// 线程ID
    uint32_t m_threadId = 0;
    /// 协程ID
    uint32_t m_fiberId = 0;
    /// 时间戳
    uint64_t m_time = 0;
    /// 线程名称
    std::string m_threadName;
    /// 日志内容流
    std::stringstream m_ss;
};

/**
 * @brief   日志事件包装类
 */
class LogEventWrap
{
public:
    LogEventWrap(LogEvent::ptr e);

    ~LogEventWrap();

    LogEvent::ptr getEvent() const { return m_event; }

    std::stringstream& getSS();

private:
    LogEvent::ptr m_event;
};

/**
 * @brief   日志格式类
 */
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

public:
    /**
    * @brief   日志内容格式化项
    */
    class FormatItem
    {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        virtual ~FormatItem() {}

        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();

    bool isError() const { return m_error; }

    const std::string getPattern() const { return m_pattern; }

private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error;
};

/**
 * @brief   日志输出目的地类
 */
class LogAppender
{
    friend class Logger;
    
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;
    /// 是否有自己的日志格式器
    bool m_hasFormatter = false;
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;
};

/**
 * @brief   标准化输出类
 */
class StdoutLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;
};

/**
 * @brief   文件输出类
 */
class FileLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string& filename);

    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_filestream;
    /// 上次重新打开的时间
    uint64_t m_lastTime;
};

/**
 * @brief   日志器类
 */
class Logger : public std::enable_shared_from_this<Logger>
{
    friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string& name = "root"); 
    
    void log(LogLevel::Level level, LogEvent::ptr event); 
    void debug(LogEvent::ptr event); 
    void info(LogEvent::ptr event); 
    void warn(LogEvent::ptr event); 
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();

    void setLevel(LogLevel::Level val) { m_level = val; }
    LogLevel::Level getLevel() const { return m_level; }
    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();
    const std::string& getName() const { return m_name; }
    
    std::string toYamlString();

private:
    std::string m_name;
    LogLevel::Level m_level;
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

/**
 * @brief   日志管理器类
 */
class LoggerManager
{
public:
    typedef Spinlock MutexType;

    LoggerManager();

    void init();

    Logger::ptr getLogger(const std::string& name);
    Logger::ptr getRoot() const { return m_root; }

    std::string toYamlString();

private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

};

#endif
