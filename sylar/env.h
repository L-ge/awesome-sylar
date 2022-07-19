/**
 * @filename    env.h
 * @brief   环境变量模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-19
 */
#ifndef __SYLAR_ENV_H__
#define __SYLAR_ENV_H__
              
#include "sylar/singleton.h"
#include "sylar/mutex.h"
#include <iostream>
#include <map>
#include <vector>
              
namespace sylar
{             
              
class Env     
{             
public:       
    typedef RWMutex RWMutexType;
              
    bool init(int argc, char** argv);
              
    void add(const std::string& key, const std::string& val);
    bool has(const std::string& key);
    void del(const std::string& key);
    std::string get(const std::string& key, const std::string& default_value = "");
              
    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();
              
    const std::string& getExe() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }
              
    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_value = "");
              
    std::string getAbsolutePath(const std::string& path) const;
    std::string getAbsoluteWorkPath(const std::string& path) const;
    std::string getConfigPath();

private:
    RWMutexType m_mutex;
    /// 参数信息
    std::map<std::string, std::string> m_args;
    /// 帮助信息
    std::vector<std::pair<std::string, std::string> > m_helps;
    /// 进程名(其实是进程信息，因为是argv[0]的值)
    std::string m_program;
    /// 即/proc/$pid/exe的值
    std::string m_exe;
    /// 根据/proc/$pid/exe的值设置的
    std::string m_cwd;
};            
              
typedef sylar::Singleton<Env> EnvMgr;

}
#endif
