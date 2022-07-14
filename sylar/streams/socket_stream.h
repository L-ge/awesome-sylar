/**
 * @filename    socket_stream.h
 * @brief   Socket流式接口的封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-14
 */
#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "sylar/stream.h"
#include "sylar/socket.h"
#include "sylar/mutex.h"
#include "sylar/iomanager.h"

namespace sylar
{

class SocketStream : public Stream
{
public:
    typedef std::shared_ptr<SocketStream> ptr;

    /**
     * @brief   构造函数
     *
     * @param   sock    Socket类
     * @param   owner   是否完全控制
     */
    SocketStream(Socket::ptr sock, bool owner = true);
    
    /**
     * @brief   析构函数（如果m_owner=true,则close）
     */
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }
    bool isConnected() const;

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    std::string getRemoteAddressString();
    std::string getLocaloAddressString();

protected:
    /// Socket类
    Socket::ptr m_socket;
    /// 是否主控(主要是是否交由该类来控制关闭)
    bool m_owner;
};

}

#endif
