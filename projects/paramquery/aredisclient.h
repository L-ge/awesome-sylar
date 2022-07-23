#ifndef AREDISCLIENT_H
#define AREDISCLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <hiredis/hiredis.h>

class ARedisClient
{
public:
    typedef std::shared_ptr<redisReply> ReplyPtr;

    ARedisClient(std::string sServerIp, int nServerPort, uint64_t nTimeOutConnectMsec = 5000);
    ~ARedisClient();

    bool connectToServer();
    bool disconnectToServer();
    ReplyPtr execRedisCommand(const std::string &sRedisCommand);
    int execRedisCommandPipeline(const std::vector<std::string> &vecRedisCommand, std::vector<ReplyPtr> &vecRedisReply);

protected:
    bool isServerOnline();

private:
    std::string m_sServerIp;
    int m_nServerPort;
    uint64_t m_nTimeOutConnectMsec;
    redisContext* m_pRedisConnect;
};

#endif // AREDISCLIENT_H

