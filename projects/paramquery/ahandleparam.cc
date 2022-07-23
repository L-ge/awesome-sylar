#include "ahandleparam.h"
#include "sylar/log.h"
#include "sylar/env.h"
#include "sylar/util.h"
#include <chrono>
#include <fstream>
#include <algorithm>

namespace paramquery
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

std::vector<std::string> split(std::string str,std::string pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;//扩展字符串以方便操作
    int size = str.size();
    for(int i=0; i<size; ++i)
    {
        pos = str.find(pattern,i);
        if((int)pos < size)
        {
            std::string s = str.substr(i, pos-i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result;
}

void encodeValue(const std::string& sStatus, const std::string& sType, std::string& sResult)
{
    unsigned long nStatus = std::stoul(sStatus);
    unsigned long nType = std::stoul(sType);
    unsigned long nHashValue = 0x00;
    nHashValue = (nStatus << 6) & 0xC0;
    nHashValue |= (nType & 0x3F);
    sResult = std::to_string(nHashValue);
}

void decodeValue(std::string& sStatus, std::string& sType, const std::string& sResult)
{
    unsigned long nHashValue = std::stoul(sResult);
    unsigned long nStatus = (nHashValue & 0xC0) >> 6;
    unsigned long nType = nHashValue & 0x3F;
    sStatus = std::to_string(nStatus);
    sType = std::to_string(nType);
}

// BKDR Hash Function
unsigned int BKDRHash(const char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

AHandleParam::AHandleParam()
{
    if(0 != startRedisServer())
    {
        SYLAR_LOG_ERROR(g_logger) << "start redis server fail";
        // TODO 抛异常？
    }

    m_pRedisClient = new ARedisClient("127.0.0.1", 6379);

    // 等redis初始化完成，等100*10ms
    for(int i=0; i<100; ++i)
    {
        if(false == m_pRedisClient->connectToServer())
        {
            SYLAR_LOG_ERROR(g_logger) << "connect redis server fail";
            // TODO 抛异常？
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        else
        {
            if(0 != startLoadParamToRedis())
                SYLAR_LOG_ERROR(g_logger) << "load param to redis fail";
            break;
        }
    }

    m_bStopThreadCheckUpdateParam = false;
    m_threadCheckUpdateParam = std::thread(&AHandleParam::checkUpdateParam, this);
}

AHandleParam::~AHandleParam()
{
    m_bStopThreadCheckUpdateParam = true;
    if(m_threadCheckUpdateParam.joinable())
    {
        m_threadCheckUpdateParam.join();
    }

    if(nullptr != m_pRedisClient)
    {
        delete m_pRedisClient;
        m_pRedisClient = nullptr;
    }
}

int AHandleParam::queryParam(const std::string& sQueryCond, std::string& sQueryResult)
{
    auto start = std::chrono::steady_clock::now();
    
    // 解析查询条件
    std::map<std::string, std::string> mpQueryCond = getQueryCond(sQueryCond);    
    if(mpQueryCond.count("cardnet") <= 0 || mpQueryCond.count("cardid") <= 0)
    {
        SYLAR_LOG_INFO(g_logger) << "param miss";
        return -1;
    }
    // 查询参数
    std::string sQueryCardId = mpQueryCond["cardnet"] + mpQueryCond["cardid"];
    SYLAR_LOG_INFO(g_logger) << "QueryCardId:" << sQueryCardId;
    std::map<std::string, std::string> mpResult = queryCardId(sQueryCardId);

    // 组装查询结果
    Json::Value json;
    for(auto& i : mpResult)     // 用于测试
    {
        json[i.first] = i.second;
    }
    
    // 根据协议的返回
    json["cmdtype"] = "blacklistqueryresult";
    json["errorcode"] = "0";
    json["serialno"] = mpQueryCond["serialno"];
    json["version"] = mpQueryCond["version"];
    if(mpResult.empty() || "2" == mpResult["status"])
    {
        json["recordcnt"] = "0";
    }
    else
    {
        json["recordcnt"] = "1";
        json["darkstatus0"] = mpResult["status"];
        json["darktype0"] = mpResult["type"];
        json["darkver0"] = "1970-01-01 08:00:00";
    }
    sQueryResult = sylar::JsonUtil::ToString(json);
    
    int nCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- start).count();
    SYLAR_LOG_INFO(g_logger) << "queryParam costMs:" << nCostMs;
    return 0;
}

int AHandleParam::startRedisServer()
{
    auto start = std::chrono::steady_clock::now();

    pid_t pid = fork();
    if(0 == pid)
    {
        std::string sRedisServerExe = sylar::EnvMgr::GetInstance()->getCwd() + "redis/redis-server"; 
        std::string sRedisServerConf = sylar::EnvMgr::GetInstance()->getCwd() + "redis/redis.conf";
        SYLAR_LOG_INFO(g_logger) << "start:" << sRedisServerExe
            << " " << sRedisServerConf;

        if(-1 == execl(sRedisServerExe.c_str(), "redis-server", sRedisServerConf.c_str(), NULL))
        {
            SYLAR_LOG_ERROR(g_logger) << "execl fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno);
            return -1;
        }
        return 0;
    }
    else if(pid < 0)
    {
        SYLAR_LOG_ERROR(g_logger) << "fork fail return=" << pid
            << " errno=" << errno << " errstr=" << strerror(errno);
        return -1;
    }
    else
    {
        int nCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- start).count();
        SYLAR_LOG_INFO(g_logger) << "startRedisServer costMs:" << nCostMs;
    }
    return 0;
}

void AHandleParam::checkUpdateParam()
{
    while(false == m_bStopThreadCheckUpdateParam)
    {
        // 检查指定目录是否有参数要更新
        std::string sDataPath = sylar::EnvMgr::GetInstance()->getCwd() + "data/";
        //SYLAR_LOG_INFO(g_logger) << "DataPath:" << sDataPath;
        std::vector<std::string> files;
        sylar::FSUtil::ListAllFile(files, sDataPath, ".txt");
        std::sort(files.begin(), files.end());
        for(auto& i : files)
        {
            SYLAR_LOG_INFO(g_logger) << "DataParam:" << i;
            prefixParam(i);
            sylar::FSUtil::Rm(i);
            std::string sPrefixParamFileName = i.substr(0, i.length()-3) + "dat";
            loadParamToRedis(sPrefixParamFileName);
        }

        for(int i=0; i<50; ++i)     // 休眠50*100ms
        {
            if(false == m_bStopThreadCheckUpdateParam)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

int AHandleParam::prefixParam(const std::string& sAbsoluteFilePath)
{
    std::ifstream ifParamFile(sAbsoluteFilePath.c_str());
    if(false == ifParamFile.is_open())
    {
        SYLAR_LOG_ERROR(g_logger) << "Open Param Fail, ParamName:" << sAbsoluteFilePath;
        return -1;
    }

    std::string sPrefixParamFileName = sAbsoluteFilePath.substr(0, sAbsoluteFilePath.length()-3) + "dat";
    std::ofstream ofPrefixParamFile;
    ofPrefixParamFile.open(sPrefixParamFileName, std::ios::app);
    
    std::hash<std::string> str_hash;
    
    auto start = std::chrono::steady_clock::now();
    std::string sLine;
    getline(ifParamFile, sLine);    // 去掉表头
    while(getline(ifParamFile, sLine))
    {
        std::vector<std::string> vecLine = split(sLine, "\t");
        std::string sKey = vecLine.at(0);
        std::string sBucketId = std::to_string(str_hash(sKey) % 300000);
        std::string sValue = "";
        encodeValue(vecLine.at(2), vecLine.at(1), sValue);
    
        unsigned int nFieldKey = BKDRHash(sKey.c_str());
        // windows下用\n，Linux下用\r\n，否则有如下报错：
        // ERR Protocol error: invalid multibulk length
        std::string sRedisProtocolCmd = std::string("*4\r\n")
            + "$4\r\n"
            + "hset\r\n"
            + "$" + std::to_string(sBucketId.length()) + "\r\n"
            + sBucketId + "\r\n"
            + "$" + std::to_string(std::to_string(nFieldKey).length()) + "\r\n"
            + std::to_string(nFieldKey) +"\r\n"
            + "$" + std::to_string(sValue.length()) + "\r\n"
            + sValue +"\r\n";
    
        ofPrefixParamFile << sRedisProtocolCmd;
    }
    ofPrefixParamFile.close();
    ifParamFile.close();
    
    int nCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- start).count();
    SYLAR_LOG_INFO(g_logger) << "prefixParam costMs:" << nCostMs;
    return 0;
}

int AHandleParam::loadParamToRedis(const std::string& sAbsoluteFilePath)
{
    auto start = std::chrono::steady_clock::now();
    SYLAR_LOG_INFO(g_logger) << "PrefixFile:" << sAbsoluteFilePath;
    std::string sPipeCmd = "cat " + sAbsoluteFilePath + " | redis-cli --pipe";
    FILE* fp = popen(sPipeCmd.c_str(), "r");
    if(NULL == fp)
    {
        SYLAR_LOG_ERROR(g_logger) << "loadParamToRedis Error";
        return -1;
    }
    char buffer[1024];
    // TODO 处理返回的结果是否error非零
    while(NULL != fgets(buffer, sizeof(buffer), fp))
        SYLAR_LOG_INFO(g_logger) << "pipe to redis result:" << buffer;
    pclose(fp);
    
    int nCostMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()- start).count();
    SYLAR_LOG_INFO(g_logger) << "loadParamToRedis costMs:" << nCostMs;
    return 0;
}

int AHandleParam::startLoadParamToRedis()
{
    std::string sDataPath = sylar::EnvMgr::GetInstance()->getCwd() + "data/";
    SYLAR_LOG_INFO(g_logger) << "DataPath:" << sDataPath;
    std::vector<std::string> files;
    sylar::FSUtil::ListAllFile(files, sDataPath, ".dat");
    std::sort(files.begin(), files.end());
    for(auto& i : files)
    {
        if(0 != loadParamToRedis(i))
        {
            return -1;
        }
    }
    return 0;
}

std::map<std::string, std::string> AHandleParam::queryCardId(const std::string& sCardId)
{
    std::hash<std::string> str_hash;
    std::string sRedisCmd = "hget " 
                          + std::to_string(str_hash(sCardId) % 300000)
                          + " "
                          + std::to_string(BKDRHash(sCardId.c_str()));
    ARedisClient::ReplyPtr pReplyPtr = m_pRedisClient->execRedisCommand(sRedisCmd);
    if(nullptr == pReplyPtr)
    {
        SYLAR_LOG_ERROR(g_logger) << "queryParam error:" << sRedisCmd;
        return {};
    }

    if(REDIS_REPLY_STRING == pReplyPtr->type)
    {
        SYLAR_LOG_INFO(g_logger) << "queryParam result:" << pReplyPtr->str;
        std::string sStatus;
        std::string sType;
        decodeValue(sStatus, sType, pReplyPtr->str);

        std::map<std::string, std::string> mpResult;
        mpResult["status"] = sStatus;
        mpResult["type"] = sType;
        return mpResult;
    }

    return {};
}

std::map<std::string, std::string> AHandleParam::getQueryCond(const std::string& sQueryCond)
{
    std::map<std::string, std::string> mpQueryCond;
    std::vector<std::string> vecQueryCond = split(sQueryCond, "&");
    for(auto& i : vecQueryCond)
    {
        auto nPos = i.find("=");
        mpQueryCond[i.substr(0, nPos)] = i.substr(nPos + 1, i.length());
    }
    return mpQueryCond;
}

}

