#ifndef __PARAMQUERY_SERVLET_H__
#define __PARAMQUERY_SERVLET_H__

#include "sylar/http/servlet.h"
#include "ahandleparam.h"

namespace sylar 
{

namespace http 
{

class ParamQueryServlet : public sylar::http::Servlet
{
public:
    typedef std::shared_ptr<ParamQueryServlet> ptr;
    
    ParamQueryServlet(const std::string& sPath);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session) override;

private:
    std::string m_sPath;
    paramquery::AHandleParam::ptr m_pHandleParam;
};

}

}

#endif

