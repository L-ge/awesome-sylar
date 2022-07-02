#ifndef __SYLAR_FDMANAGER_H__
#define __SYLAR_FDMANAGER_H__

#include <memory>
#include <vector>
#include "mutex.h"
#include "singleton.h"

namespace sylar
{

/**
 * @brief   文件描述符上下文类(其实就是对文件描述符一些属性的封装)
 */
class FdCtx : public std::enable_shared_from_this<FdCtx>
{
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();

    bool isInit() const { return m_isInit; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }

    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getUserNonblock() { return m_userNonblock; }
    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() { return m_sysNonblock; }

    /**
     * @brief  设置超时时间 
     *
     * @param   type    类型：SO_RCVTIMEO(读超时)、SO_SNDTIMEO(写超时)
     * @param   v
     */
    void setTimeout(int type, uint64_t v);
    
    /**
     * @brief   获取超时时间
     *
     * @param   type    类型：SO_RCVTIMEO(读超时)、SO_SNDTIMEO(写超时)
     */
    uint64_t getTimeout(int type);

private:
    bool init();

private:
    /// 是否初始化
    bool m_isInit:1;
    /// 是否是socket
    bool m_isSocket:1;
    /// 是否 hook 非阻塞
    bool m_sysNonblock:1;
    /// 是否用户主动设置非阻塞
    bool m_userNonblock:1;
    /// 是否关闭
    bool m_isClosed:1;
    /// 文件描述符
    int m_fd;
    /// 读超时时间
    uint64_t m_recvTimeout;
    /// 写超时时间
    uint64_t m_sendTimeout;
};

/**
 * @brief   文件描述符管理类
 */
class FdManager
{
public:
    typedef RWMutex RWMutexType;

    FdManager();

    /**
     * @brief   获取/创建文件描述符类 FdCtx
     *
     * @param   fd  文件描述符
     * @param   auto_create 是否自动创建
     *
     * @return  返回对应文件描述符类 FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;     // 文件描述符管理器单例

}

#endif
