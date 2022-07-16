#include "http_server.h"
#include "sylar/log.h"
//#include "sylar/http/servlets/config_servlet.h"
//#include "sylar/http/servlets/status_servlet.h"

namespace sylar
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace http
{

HttpServer::HttpServer(bool keepalive
                     , sylar::IOManager* worker
                     , sylar::IOManager* io_worker
                     , sylar::IOManager* accept_worker)
    : TcpServer(worker, io_worker, accept_worker)
    , m_isKeepalive(keepalive)
{
    m_dispatch.reset(new ServletDispatch);

    m_type = "http";
    //m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    //m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string& v)
{
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client)
{
    SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do
    {
        auto req = session->recvRequest();
        if(!req)
        {
            SYLAR_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " client:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                    , req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);  // 交给ServletDispatch处理
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose())
        {
            break;
        }
    } while(true);

    session->close();
}

}

}
