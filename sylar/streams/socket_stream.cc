#include "socket_stream.h"

namespace sylar
{

SocketStream::SocketStream(Socket::ptr sock, bool owner)
    : m_socket(sock)
    , m_owner(owner)
{
}

SocketStream::~SocketStream()
{
    if(m_owner && m_socket)
    {
        m_socket->close();
    }
}

int SocketStream::read(void* buffer, size_t length)
{
    if(!isConnected())
    {
        return -1;
    }
    return m_socket->recv(buffer, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length)
{
    if(!isConnected())
    {
        return -1;
    }
    
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);      // 拿到iovec写缓存
    int rt = m_socket->recv(&iovs[0], iovs.size());
    if(rt > 0)
    {
        ba->setPosition(ba->getPosition() + rt);    // 更新当前操作位置
    }
    return rt;
}

int SocketStream::write(const void* buffer, size_t length)
{
    if(!isConnected())
    {
        return -1;
    }
    return m_socket->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length)
{
    if(!isConnected())
    {
        return -1;
    }

    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);       // 拿到iovec读缓存
    int rt = m_socket->send(&iovs[0], iovs.size());
    if(rt > 0)
    {
        ba->setPosition(ba->getPosition() + rt);    // 更新当前操作位置
    }
    return rt;
}

void SocketStream::close()
{
    if(m_socket)
    {
        m_socket->close();
    }
}

bool SocketStream::isConnected() const
{
    return m_socket && m_socket->isConnected();
}

Address::ptr SocketStream::getRemoteAddress()
{
    if(m_socket)
    {
        return m_socket->getRemoteAddress();
    }
    return nullptr;
}

Address::ptr SocketStream::getLocalAddress()
{
    if(m_socket)
    {
        return m_socket->getLocalAddress();
    }
    return nullptr;
}

std::string SocketStream::getRemoteAddressString()
{
    auto addr = getRemoteAddress();
    if(addr)
    {
        return addr->toString();
    }
    return "";
}

std::string SocketStream::getLocaloAddressString()
{
    auto addr = getLocalAddress();
    if(addr)
    {
        return addr->toString();
    }
    return "";
}

}
