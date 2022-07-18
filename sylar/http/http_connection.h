/**
 * @filename    http_connection.h
 * @brief   http客户端类
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-18
 */
#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include <list>
#include "sylar/streams/socket_stream.h"
#include "http.h"
#include "sylar/uri.h"
#include "sylar/mutex.h"

namespace sylar
{

namespace http
{

/**
 * @brief   http响应结果
 */
struct HttpResult 
{
    typedef std::shared_ptr<HttpResult> ptr;
   
    /**
     * @brief   错误码定义
     */
    enum class Error 
    {
        /// 正常
        OK                      = 0,
        /// 非法URL
        INVALID_URL             = 1,
        /// 无法解析HOST
        INVALID_HOST            = 2,
        /// 连接失败
        CONNECT_FAIL            = 3,
        /// 连接被对端关闭
        SEND_CLOSE_BY_PEER      = 4,
        /// 发送请求产生Socket错误
        SEND_SOCKET_ERROR       = 5,
        /// 超时
        TIMEOUT                 = 6,
        /// 创建Socket失败
        CREATE_SOCKET_ERROR     = 7,
        /// 从连接池中取连接失败
        POOL_GET_CONNECTION     = 8,
        /// 无效的连接
        POOL_INVALID_CONNECTION = 9,
    };

    /**
     * @brief   构造函数  
     *
     * @param   _result 错误码
     * @param   _response   http响应结构体
     * @param   _error      错误描述
     */
    HttpResult(int _result
             , HttpResponse::ptr _response
             , const std::string& _error)
        : result(_result)
        , response(_response)
        , error(_error)
    {}

    std::string toString() const;
    
    /// 错误码
    int result;
    /// HTTP响应结构体
    HttpResponse::ptr response;
    /// 错误描述
    std::string error;
};

class HttpConnectionPool;

/**
 * @brief   http客户端类
 */
class HttpConnection : public SocketStream
{

friend class HttpConnectionPool;

public:
    typedef std::shared_ptr<HttpConnection> ptr;

    static HttpResult::ptr DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");
    
    static HttpResult::ptr DoPost(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    /**
     * @brief   发送http请求
     *
     * @param   method      请求类型
     * @param   url         请求的url
     * @param   timeout_ms  超时时间(毫秒)
     * @param   headers     http请求头部参数
     * @param   body        请求消息体
     *
     * @return  返回http结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    /**
     * @brief   发送http请求
     *
     * @param   method      请求类型
     * @param   uri         URI 结构体
     * @param   timeout_ms  超时时间(毫秒)
     * @param   headers     http请求头部参数
     * @param   body        请求消息体
     *
     * @return  返回http结果结构体
     */
    static HttpResult::ptr DoRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                            , Uri::ptr uri
                            , uint64_t timeout_ms);

    HttpConnection(Socket::ptr sock, bool owner = true);
    ~HttpConnection();

    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;
};

class HttpConnectionPool 
{
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;

    static HttpConnectionPool::ptr Create(const std::string& uri
                                   , const std::string& vhost
                                   , uint32_t max_size
                                   , uint32_t max_alive_time
                                   , uint32_t max_request);

    HttpConnectionPool(const std::string& host
                       , const std::string& vhost
                       , uint32_t port
                       , bool is_https
                       , uint32_t max_size
                       , uint32_t max_alive_time
                       , uint32_t max_request);

    HttpConnection::ptr getConnection();

    HttpResult::ptr doGet(const std::string& url
                          , uint64_t timeout_ms
                          , const std::map<std::string, std::string>& headers = {}
                          , const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri
                           , uint64_t timeout_ms
                           , const std::map<std::string, std::string>& headers = {}
                           , const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url
                           , uint64_t timeout_ms
                           , const std::map<std::string, std::string>& headers = {}
                           , const std::string& body = "");

    HttpResult::ptr doPost(Uri::ptr uri
                           , uint64_t timeout_ms
                           , const std::map<std::string, std::string>& headers = {}
                           , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                            , const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                            , Uri::ptr uri
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers = {}
                            , const std::string& body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req
                            , uint64_t timeout_ms);

private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;
    uint32_t m_maxAliveTime;
    uint32_t m_maxRequest;
    bool m_isHttps;

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};

}

}

#endif
