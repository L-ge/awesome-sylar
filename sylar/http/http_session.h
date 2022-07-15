/**
 * @filename    http_session.h
 * @brief   HttpSession的封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-15
 */
#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "sylar/streams/socket_stream.h"
#include "http.h"

namespace sylar
{

namespace http
{

class HttpSession : public SocketStream
{
public:
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief   构造函数 
     *
     * @param   sock    Socket类型
     * @param   owner   是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief   接受http请求
     */
    HttpRequest::ptr recvRequest();
    
    /**
     * @brief  发送http响应 
     *
     * @param   rsp http响应
     *
     * @return  >0 发送成功
     *          =0 对方关闭
     *          <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}

}

#endif
