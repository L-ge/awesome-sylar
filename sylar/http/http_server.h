/**
 * @filename    http_server.h
 * @brief   Http服务器的封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-16
 */
#ifndef __SYLAR_HTTP_HTTP_SERVER_H
#define __SYLAR_HTTP_HTTP_SERVER_H

#include "sylar/tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace sylar
{

namespace http
{

class HttpServer : public TcpServer
{
public:
    typedef std::shared_ptr<HttpServer> ptr;

    /**
     * @brief   构造函数
     *
     * @param   keepalive   是否长连接
     * @param   worker      工作调度器
     * @param   io_worker
     * @param   accept_worker   接收连接的调度器
     */
    HttpServer(bool keepalive = false
             , sylar::IOManager* worker = sylar::IOManager::GetThis()
             , sylar::IOManager* io_worker = sylar::IOManager::GetThis()
             , sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

    /**
     * @brief   获取ServletDispatch
     */
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

    /**
     * @brief   设置ServletDispatch 
     */
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

    virtual void setName(const std::string& v) override;

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}

}

#endif
