#include "log.h"
#include "config.h"

namespace sylar
{

const char* LogLevel::ToString(LogLevel::Level level)
{
    switch(level)
    {
#define XX(name) \
        case LogLevel::name: \
            return #name; \
            break;

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
#undef XX
        default:
            return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str)
{
#define XX(level, v) \
    if(str == #v) \
    { \
        return LogLevel::level; \
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;
#undef XX
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, 
        LogLevel::Level level,
        const char* file,
        int32_t line,
        uint32_t elapse,
        uint32_t threadId,
        uint32_t fiberId,
        uint64_t time,
        const std::string& threadName)
    : m_logger(logger)
    , m_level(level)
    , m_file(file)
    , m_line(line)
    , m_elapse(elapse)
    , m_threadId(threadId)
    , m_fiberId(fiberId)
    , m_time(time)
    , m_threadName(threadName)
{
}

void LogEvent::format(const char* fmt, ...)
{
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al)
{
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1)
    {
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    : m_event(e)
{

}

LogEventWrap::~LogEventWrap()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS()
{
    return m_event->getSS();
}

/**
 * @brief   m:消息
 */
class  MessageFormatItem : public LogFormatter::FormatItem
{
public:
    MessageFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getContent();
    }
};

/**
 * @brief   p:日志级别
 */
class LevelFormatItem : public LogFormatter::FormatItem
{
public:
    LevelFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << LogLevel::ToString(level);
    }
};

/**
 * @brief   r:累计毫秒数
 */
class ElapseFormatItem : public LogFormatter::FormatItem
{
public:
    ElapseFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getElapse();
    }
};

/**
 * @brief   c:日志名称
 */
class NameFormatItem : public LogFormatter::FormatItem
{
public:
    NameFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getLogger()->getName();
    }
};

/**
 * @brief   t:线程id
 */
class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
    ThreadIdFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getThreadId();
    }
};

/**
 * @brief   n:换行
 */
class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
    NewLineFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << std::endl;
    }
};

/**
 * @brief   d:时间
 */
class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        : m_format(format)
    {
        if(m_format.empty())
        {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }

private:
    std::string m_format;
};

/**
 * @brief   f:文件名
 */
class FilenameFormatItem : public LogFormatter::FormatItem
{
public:
    FilenameFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getFile();
    }
};

/**
 * @brief   l:行号
 */
class LineFormatItem : public LogFormatter::FormatItem
{
public:
    LineFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getLine();
    }
};

/**
 * @brief   T:Tab
 */
class TabFormatItem : public LogFormatter::FormatItem
{
public:
    TabFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << "\t";
    }
};

/**
 * @brief   F:协程id
 */
class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
    FiberIdFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getFiberId();
    }
};

/**
 * @brief   N:线程名称
 */
class ThreadNameFormatItem : public LogFormatter::FormatItem
{
public:
    ThreadNameFormatItem(const std::string& str = "") {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << event->getThreadName();
    }
};

/**
 * @brief   直接打印字符串
 */
class StringFormatItem : public LogFormatter::FormatItem
{
public:
    StringFormatItem(const std::string& str)
        : m_string(str)
    {}

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        os << m_string;
    }

private:
    std::string m_string;
};

LogFormatter::LogFormatter(const std::string& pattern)
    : m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    std::stringstream ss;
    for(auto& i : m_items)
    {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    for(auto& i : m_items)
    {
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

/**
 * @brief   该日志器逻辑最复杂的函数：对日志样式进行解析。有如下四种格式：
 *          %xxx        是对xxx进行解析
 *          %xxx{yyy}   是对xxx进行解析，它是yyy这种格式的
 *          %%          是对%进行转义
 *          还有一种是直接要打印的字符串，比如[]这种。
 */
void LogFormatter::init()  //%xxx %xxx{xxx} %%
{
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;    // 存放解析后的样式
    std::string nstr;                                               // 存放实实在在的、并不是样式的字符串
    for(size_t i = 0; i < m_pattern.size(); ++i)    // 循环解析每一个字符
    {
        if(m_pattern[i] != '%')         // 当前字符不是%，就把它缓存到nstr
        {
            nstr.append(1, m_pattern[i]);
            continue;
        }
        // 下面是等于%的情况


        // 第i个和第i+1个都是%，那么就是转义，压一个%进nstr 
        if((i + 1) < m_pattern.size()) 
        {
            if(m_pattern[i + 1] == '%') 
            {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;               // 第i个是%，那么从i+1个字符开始解析
        int fmt_status = 0;             // 解析方式：0是不需要解析格式，1是解析格式
        size_t fmt_begin = 0;

        std::string str;                // %xxx{yyy}中的xxx
        std::string fmt;                // %xxx{yyy}中的yyy
        while(n < m_pattern.size())     // 保证不越界
        {
            if(!fmt_status
                    && (!isalpha(m_pattern[n])
                        && m_pattern[n] != '{'
                        && m_pattern[n] != '}'))    // 表示解析的就是%xxx格式，于是结束这一段样式的解析
            {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }

            if(fmt_status == 0) 
            {
                if(m_pattern[n] == '{') // 需要解析格式
                {
                    str = m_pattern.substr(i + 1, n - i - 1);  // str即为xxx{yyy}中xxx
                    fmt_status = 1;     // 解析格式
                    fmt_begin = n;      // 记录需要解析格式的起始位置，即xxx{yyy}中的{的位置
                    ++n;
                    continue;
                }
            }
            else if(fmt_status == 1)    // 需要解析格式
            {
                if(m_pattern[n] == '}') // 结束yyy的查找了
                {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);   // fmt即为yyy
                    fmt_status = 0;     // 重置状态为不需要解析格式
                    ++n;
                    break;              // 结束这一段样式的解析了
                }
            }
            ++n;
            if(n == m_pattern.size())   // 已经到样式结尾了
            {
                if(str.empty())         // 如果样式为空的话，则样式即为第i+1个字符一直到结束，也就是只有一个样式或已经到了最后一个样式才有这种情况 
                {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0)
        {
            if(!nstr.empty())   // 如果nstr不为空，则证明是nstr是要打印的字符串，所以以string()的样式存到vector里面去
            {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;          // n-1再进入下一轮循环
        }
        else if(fmt_status == 1) // 因为上面会重置，因此此时还是1，证明解析样式有错误
        {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    } // 这里已经是解析完成

    if(!nstr.empty())   // 解析完成后，nstr有值，证明其是要打印的字符串，则以string的形式存到vector里面去
    {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),           // m:消息 
        XX(p, LevelFormatItem),             // p:日志级别
        XX(r, ElapseFormatItem),            // r:累计毫秒数
        XX(c, NameFormatItem),              // c:日志名称
        XX(t, ThreadIdFormatItem),          // t:线程id
        XX(n, NewLineFormatItem),           // n:换行
        XX(d, DateTimeFormatItem),          // d:时间
        XX(f, FilenameFormatItem),          // f:文件名
        XX(l, LineFormatItem),              // l:行号
        XX(T, TabFormatItem),               // T:Tab
        XX(F, FiberIdFormatItem),           // F:协程id
        XX(N, ThreadNameFormatItem),        // N:线程名称
#undef XX
    };

    for(auto& i : vec) 
    {
        if(std::get<2>(i) == 0)         // 也就是type为0，证明要打印的是字符串，并不是样式，因此解析为StringFormatItem，直接打印即可。
        {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } 
        else 
        {
            auto it = s_format_items.find(std::get<0>(i));  // 也就是xxx{yyy}中的xxx，这正式要解析的
            if(it == s_format_items.end()) 
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            }
            else // 一般来说，fmt是空的，只有xxx{yyy}这种情况才不是空，fmt即yyy，比如时间，xxx就是d，用来new出DateTimeFormatItem，而yyy可能是hh::mm:ss这种。
            {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}

void LogAppender::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lk(m_mutex);
    m_formatter = val;
    if(m_formatter = val)
    {
        m_hasFormatter = true;
    }
    else
    {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter()
{
    MutexType::Lock lk(m_mutex);
    return m_formatter;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        MutexType::Lock lk(m_mutex);
        m_formatter->format(std::cout, logger, level, event);
    }
}

std::string StdoutLogAppender::toYamlString()
{
    MutexType::Lock lk(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW)
    {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename)
{
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        /// 超过3ms就重新打开一次
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3))
        {
            reopen();
            m_lastTime = now;
        }

        MutexType::Lock lk(m_mutex);
        if(!m_formatter->format(m_filestream, logger, level, event))
        {
            std::cout << "error" << std::endl;
        }
    }
}

std::string FileLogAppender::toYamlString()
{
    MutexType::Lock lk(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW)
    {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen()
{
    MutexType::Lock lk(m_mutex);
    if(m_filestream)
    {
        m_filestream.close();
    }
    return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
}

Logger::Logger(const std::string& name)
    : m_name(name)
    , m_level(LogLevel::DEBUG) 
{
    //m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%m%n"));
}
    
void Logger::log(LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        auto self = shared_from_this();
        MutexType::Lock lk(m_mutex);
        if(!m_appenders.empty())
        {
            for(auto& i : m_appenders)
            {
                i->log(self, level, event);
            }
        }
        else if(m_root)
        {
            m_root->log(level, event);
        }
    }
}
    
void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}
    
void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}
    
void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}
    
void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lk(m_mutex);
    if(!appender->getFormatter())
    {
        MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lk(m_mutex);
    for(auto it = m_appenders.begin();
            it != m_appenders.end();
            ++it)
    {
        if(*it == appender)
        {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppenders()
{
    MutexType::Lock lk(m_mutex);
    m_appenders.clear();
}

void Logger::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lk(m_mutex);
    m_formatter = val;
    for(auto& i : m_appenders)
    {
        MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter)
        {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val)
{
    LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->isError())
    {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val 
                  << " invalid formatter\n";
        return;
    }
    setFormatter(new_val);
}

LogFormatter::ptr Logger::getFormatter()
{
    MutexType::Lock lk(m_mutex);
    return m_formatter;
}
    
std::string Logger::toYamlString()
{
    MutexType::Lock lk(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW)
    {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter)
    {
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto& i : m_appenders)
    {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->m_name] = m_root;

    init();
}

void LoggerManager::init()
{
}

Logger::ptr LoggerManager::getLogger(const std::string& name)
{
    MutexType::Lock lk(m_mutex);
    auto it = m_loggers.find(name);
    if(it != m_loggers.end())
    {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

std::string LoggerManager::toYamlString()
{
    MutexType::Lock lk(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers)
    {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

}

