#include "env.h"
#include "sylar/config.h"
#include <iomanip>

namespace sylar
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

bool Env::init(int argc, char** argv)
{
    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, path, sizeof(path)); // 从软链接获取真正的路径
    m_exe = path;

    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos) + "/";

    m_program = argv[0];

    const char* now_key = nullptr;
    for(int i=1; i<argc; ++i)
    {
        if(argv[i][0] == '-')
        {
            if(strlen(argv[i]) > 1)
            {
                if(now_key)
                {
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            }
            else
            {
                SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                          << " val=" << argv[i];
                return false;
            }
        }
        else
        {
            if(now_key)
            {
                add(now_key, argv[i]);
                now_key = nullptr;
            }
            else
            {
                SYLAR_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                                          << " val" << argv[i];
                return false;
            }
        }
    }
    if(now_key)
    {
        add(now_key, "");
    }
    return true;
}

void Env::add(const std::string& key, const std::string& val)
{
    RWMutexType::WriteLock lk(m_mutex);
    m_args[key] = val;
}

bool Env::has(const std::string& key)
{
    RWMutexType::ReadLock lk(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}

void Env::del(const std::string& key)
{
    RWMutexType::WriteLock lk(m_mutex);
    m_args.erase(key);
}
    
std::string Env::get(const std::string& key, const std::string& default_value)
{
    RWMutexType::ReadLock lk(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? it->second : default_value;
}
              
void Env::addHelp(const std::string& key, const std::string& desc)
{
    removeHelp(key);
    RWMutexType::WriteLock lk(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key)
{
    RWMutexType::WriteLock lk(m_mutex);
    for(auto it=m_helps.begin(); it!=m_helps.end();)
    {
        if(it->first == key)
        {
            it = m_helps.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Env::printHelp()
{
    RWMutexType::ReadLock lk(m_mutex);
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    for(auto& i : m_helps)
    {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

bool Env::setEnv(const std::string& key, const std::string& val)
{
    //int setenv(const char* name, const char* value, int overwrite);
    //setenv()用来改变或增加环境变量的内容。
    //参数name为环境变量名称字符串。
    //参数value则为变量内容。
    //参数overwrite用来决定是否要改变已存在的环境变量。
    //如果没有此环境变量则无论overwrite为何值均添加此环境变量。
    //若环境变量存在，当overwrite不为0时，原内容会被改为参数value所指的变量内容；
    //当overwrite为0时，则参数value会被忽略。返回值执行成功则返回0，有错误发生时返回-1。
    return !setenv(key.c_str(), val.c_str(), 1);
}
    
std::string Env::getEnv(const std::string& key, const std::string& default_value)
{
    const char* v = getenv(key.c_str());
    if(v == nullptr)
    {
        return default_value;
    }
    return v;
}
              
std::string Env::getAbsolutePath(const std::string& path) const
{
    if(path.empty())
    {
        return "/";
    }
    if(path[0] == '/')  // 如果第一个字符已经是/，那这已经是绝对路径了。
    {
        return path;
    }
    return m_cwd + path;
}

std::string Env::getAbsoluteWorkPath(const std::string& path) const
{
    if(path.empty())
    {
        return "/";
    }
    if(path[0] == '/')
    {
        return path;
    }
    static sylar::ConfigVar<std::string>::ptr g_server_work_path = 
        sylar::Config::Lookup<std::string>("server.work_path");
    return g_server_work_path->getValue() + "/" + path;
}

std::string Env::getConfigPath()
{
    return getAbsolutePath(get("c", "conf"));
}

}
