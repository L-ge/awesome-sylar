#ifndef AHANDLEPARAM_H
#define AHANDLEPARAM_H

#include <memory>
#include <thread>
#include <map>
#include "aredisclient.h"

namespace paramquery
{

class AHandleParam
{
public:
    typedef std::shared_ptr<AHandleParam> ptr;
    
    AHandleParam();
    ~AHandleParam();

    int queryParam(const std::string& sQueryCond, std::string& sQueryResult);

private:
    // 启动redis-server
    int startRedisServer();
    // 检查指定目录下是否有参数要更新
    void checkUpdateParam();
    // 对参数进行预处理
    int prefixParam(const std::string& sAbsoluteFilePath);
    // 对预处理后的参数加载到redis
    int loadParamToRedis(const std::string& sAbsoluteFilePath);
    int startLoadParamToRedis();

    std::map<std::string, std::string> getQueryCond(const std::string& sQueryCond);
    std::map<std::string, std::string> queryCardId(const std::string& sCardId);

private:
    ARedisClient* m_pRedisClient;

    std::thread m_threadCheckUpdateParam;
    bool m_bStopThreadCheckUpdateParam;
};

#endif // AHANDLEPARAM_H

}
