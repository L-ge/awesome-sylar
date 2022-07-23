#include "aredisclient.h"
#include <cstring>
#include <iostream>

ARedisClient::ARedisClient(std::string sServerIp, int nServerPort, uint64_t nTimeOutConnectMsec)
    : m_sServerIp(sServerIp)
    , m_nServerPort(nServerPort)
    , m_nTimeOutConnectMsec(nTimeOutConnectMsec)
    , m_pRedisConnect(nullptr)
{

}

ARedisClient::~ARedisClient()
{
    if(nullptr != m_pRedisConnect)
    {
        redisFree(m_pRedisConnect);
        m_pRedisConnect = nullptr;
    }
}

bool ARedisClient::connectToServer()
{
    timeval tv = {(int) m_nTimeOutConnectMsec / 1000, ((int) m_nTimeOutConnectMsec % 1000) * 1000};
    m_pRedisConnect = redisConnectWithTimeout(m_sServerIp.c_str(), m_nServerPort, tv);

    if(nullptr == m_pRedisConnect)
    {
        std::cout << "redis connect fail" << std::endl;
        return false;
    }

    if(0 != m_pRedisConnect->err)
    {
        std::cout << "redis connect error, code:" << m_pRedisConnect->err
                  << ", errormsg:" << m_pRedisConnect->errstr << std::endl;
        return false;
    }

    std::cout << "redis connect success, serverip:" << m_sServerIp
              << ", serverport:" << m_nServerPort << std::endl;
    return true;
}

bool ARedisClient::disconnectToServer()
{
    if(nullptr == m_pRedisConnect)
    {
        return true;
    }

    redisFree(m_pRedisConnect);
    m_pRedisConnect = nullptr;

    return true;
}

ARedisClient::ReplyPtr ARedisClient::execRedisCommand(const std::string &sRedisCommand)
{
    if(nullptr == m_pRedisConnect
            || false == isServerOnline())
    {
        return nullptr;
    }

    redisReply* pReply = (redisReply*)redisCommand(m_pRedisConnect, sRedisCommand.c_str());
    if(nullptr == pReply)
    {
        std::cout << "exec rediscommand error, code:" << m_pRedisConnect->err
                  << ", errormsg:" << m_pRedisConnect->errstr
                  << ", reconnect..." << std::endl;

        redisReconnect(m_pRedisConnect);
        return nullptr;
    }

    ReplyPtr rt(pReply, freeReplyObject);
    if(pReply->type != REDIS_REPLY_ERROR)
    {
        return rt;
    }

    std::cout << "exec rediscommand return error, errormsg:"
              << pReply->str << std::endl;
    return nullptr;
}

int ARedisClient::execRedisCommandPipeline(const std::vector<std::string> &vecRedisCommand, std::vector<ReplyPtr> &vecRedisReply)
{
    if(nullptr == m_pRedisConnect
            || false == isServerOnline())
    {
        return -1;
    }

    for(const auto& sRedisCmd : vecRedisCommand)
    {
        redisAppendCommand(m_pRedisConnect, sRedisCmd.c_str());
    }

    for(size_t i=0; i<vecRedisCommand.size(); ++i)
    {
        redisReply* pReply = nullptr;
        if(REDIS_OK == redisGetReply(m_pRedisConnect, (void **)&pReply))
        {
            ReplyPtr rt(pReply, freeReplyObject);
            vecRedisReply.push_back(rt);
        }
        else
        {
            std::cout << "exec rediscommandpipeline return error, errormsg:"
                      << pReply->str << std::endl;
            freeReplyObject(pReply);
            vecRedisReply.clear();
            return -1;
        }
    }
    return 0;
}

bool ARedisClient::isServerOnline()
{
    if(nullptr == m_pRedisConnect)
    {
        return false;
    }

    const std::string sHeartCommand = "ping";
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisConnect, sHeartCommand.c_str());
    if(nullptr == pReply)
    {
        std::cout << "exec rediscommand error, code:" << m_pRedisConnect->err
                  << ", errormsg:" << m_pRedisConnect->errstr
                  << ", reconnect..." << std::endl;

        redisReconnect(m_pRedisConnect);
        return false;
    }

    if(REDIS_REPLY_ERROR == pReply->type)
    {
        std::cout << "ping redisserver return error, errormsg:"
                  << pReply->str << std::endl;
        freeReplyObject(pReply);
        return false;
    }

    freeReplyObject(pReply);
    return true;
}

