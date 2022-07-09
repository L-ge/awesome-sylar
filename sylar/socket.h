/**
 * @filename    socket.h
 * @brief   Socket 模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-09
 */
#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <openssl/err.h>
//#include <openssl/ssl.h>
#include "address.h"
#include "noncopyable.h"

namespace sylar
{

/**
 * @brief   Socket封装类
 */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable
{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    /**
     * @brief   Socket类型枚举
     */
    enum Type
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM,
    };

    /**
     * @brief   Socket协议簇枚举
     */
    enum Family
    {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX,
    };

    static Socket::ptr CreateTCP(sylar::Address::ptr address);
    static Socket::ptr CreateUDP(sylar::Address::ptr address);
    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();
    static Socket::ptr CreateTCPSocket6();
    static Socket::ptr CreateUDPSocket6();
    static Socket::ptr CreateUnixTCPSocket();
    static Socket::ptr CreateUnixUDPSocket();

    Socket(int family, int type, int protocol = 0);
    virtual ~Socket();
    
    /**
     * @brief   设置发送超时时间(毫秒) 
     */
    void setSendTimeout(int64_t v);
    int64_t getSendTimeout();
    
    /**
     * @brief   设置接受超时时间(毫秒)
     */
    void setRecvTimeout(int64_t v);
    int64_t getRecvTimeout();
    
    bool getOption(int level, int option, void* result, socklen_t* len);

    template<class T>
    bool getOption(int level, int option, T& result)
    {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int option, const void* result, socklen_t len);

    template<class T>
    bool setOption(int level, int option, const T& value)
    {
        return setOption(level, option, &value, sizeof(T));
    }

    virtual Socket::ptr accept();
    virtual bool bind(const Address::ptr addr);
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    virtual bool reconnet(uint64_t timeout_ms = -1);
    virtual bool listen(int backlog = SOMAXCONN);
    virtual bool close();

    virtual int send(const void* buffer, size_t length, int flags = 0);
    virtual int send(const iovec* buffers, size_t length, int flags = 0);
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);
    virtual int recv(void* buffer, size_t length, int flags = 0);
    virtual int recv(iovec* buffer, size_t length, int flags = 0);
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    /**
     * @brief  获取远端地址 
     */
    Address::ptr getRemoteAddress();
    
    /**
     * @brief  获取本地地址 
     */
    Address::ptr getLocalAddress();
    
    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }
    bool isConnected() const { return m_isConnected; }
    
    /**
     * @brief  是否有效（m_sock != -1）
     */
    bool isValid() const;
    int getError();

    /**
     * @brief  输出信息到流中 
     */
    virtual std::ostream& dump(std::ostream& os) const;
    virtual std::string toString() const;

    int getSocket() const { return m_sock; }
    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

protected:
    void initSock();

    /**
     * @brief   创建socket
     */
    void newSock();
    virtual bool init(int sock);

protected:
    /// socket句柄
    int m_sock;
    /// 协议簇
    int m_family;
    /// 类型
    int m_type;
    /// 协议
    int m_protocol;
    /// 是否连接
    bool m_isConnected;
    /// 本地地址
    Address::ptr m_localAddress;
    /// 远端地址
    Address::ptr m_remoteAddress;
};

/*
class SSLSocket : public Socket
{
public:
    typedef std::shared_ptr<SSLSocket> ptr;

m_isConnected    static SSLSocket::ptr CreateTCP(sylar::Address::ptr address);
    static SSLSocket::ptr CreateTCPSocket();
    static SSLSocket::ptr CreateTCPSocket6();

    SSLSocket(int family, int type, int protocol = 0);
    virtual Socket::ptr accept() override;
    virtual bool bind(const Address::ptr addr) override;
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
    virtual bool listen(int backlog = SOMAXCONN) override;
    virtual bool close() override;
    
    virtual int send(const void* buffer, size_t length, int flags = 0) override;
    virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int recv(void* buffer, size_t length, int flags = 0) override;
    virtual int recv(iovec* buffer, size_t length, int flags = 0) override;
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

    bool loadCertificates(const std::string& cert_file, const std::string& key_file);
    virtual std::ostream& dump(std::ostream& os) const override;

protected:
    virtual bool init(int sock) override;

private:
    std::shared_ptr<SSL_CTX> m_ctx;
    std::shared_ptr<SSL> m_ssl;
};
*/

/**
 * @brief   流式输出socket   
 *
 * @param   os      输出流
 * @param   sock    Socket类
 */
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}


#endif
