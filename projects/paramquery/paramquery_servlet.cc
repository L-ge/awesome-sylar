#include "paramquery_servlet.h"
#include "sylar/log.h"
#include <iostream>
#include <fstream>

namespace sylar 
{

namespace http 
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

ParamQueryServlet::ParamQueryServlet(const std::string& sPath)
    : Servlet("ParamQueryServlet")
    , m_sPath(sPath)
{
    m_pHandleParam.reset(new paramquery::AHandleParam);
}

int32_t ParamQueryServlet::handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session)
{
    auto path = m_sPath + "/" + request->getPath();
    SYLAR_LOG_INFO(g_logger) << "handle path=" << path;
    if(path.find("..") != std::string::npos) 
    {
        response->setBody("invalid path");
        response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
        return 0;
    }

    std::string sQuery = request->getQuery();
    SYLAR_LOG_INFO(g_logger) << "handle query=" << sQuery;
    
    // 查询参数
    std::string sResult = "";
    m_pHandleParam->queryParam(sQuery, sResult);
    
    // 回复结果
    std::stringstream ss;
    ss << sResult;
    response->setBody(ss.str());
    response->setHeader("content-type", "text/html;charset=utf-8");
    return 0;
}

}

}
