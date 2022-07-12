/**
 * @filename    http.h
 * @brief   http 定义结构体封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-11
 */
#ifndef __SYLAR_HTTP_HTTP_H__
#define __SYLAR_HTTP_HTTP_H__

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <boost/lexical_cast.hpp>

namespace sylar
{

namespace http
{
// copy from https://github.com/nodejs/http-parser/blob/main/http_parser.h
/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

/**
 * @brief   http 方法枚举类
 */
enum class HttpMethod
{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

/**
 * @brief   http 状态枚举类
 */
enum class HttpStatus
{
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief   将字符串方法转成http方法枚举
 */
HttpMethod StringToHttpMethod(const std::string& m);

/**
 * @brief   将字符串方法转成http方法枚举
 */
HttpMethod CharsToHttpMethod(const char* m);

/**
 * @brief   将http方法枚举转换成字符串
 */
const char* HttpMethodToString(const HttpMethod& m);

/**
 * @brief   将http状态枚举转换成字符串
 */
const char* HttpStatusTostring(const HttpStatus& s);

/**
 * @brief   忽略大小写比较的仿函数
 */
struct CaseInsensitiveLess
{
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

/**
 * @brief  获取MapType中的key值，并转换成对应的类型，返回是否成功 
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T())
{
    auto it = m.find(key);
    if(it == m.end())
    {
        val = def;
        return false;
    }
    try
    {
        val = boost::lexical_cast<T>(it->second);
        return true;
    }
    catch(...)
    {
        val = def;
    }
    return false;
}

/**
 * @brief  获取MapType中的key值，并转换成对应的类型返回 
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T())
{
    auto it = m.find(key);
    if(it == m.end())
    {
        return def;
    }
    try
    {
        return boost::lexical_cast<T>(it->second);
    }
    catch(...)
    {
    }
    return def;
}

class HttpResponse;

/**
 * @brief   http 请求结构
 */
class HttpRequest
{
public:
    typedef std::shared_ptr<HttpRequest> ptr;
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief   构造函数
     *
     * @param   version http版本号. 0x11是1.1版本，0x10是1.0版本
     * @param   close   是否keepalive
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);
    std::shared_ptr<HttpResponse> createResponse();
    
    HttpMethod getMethod() const { return m_method; }
    uint8_t getVersion() const { return m_version; }
    const std::string& getPath() const { return m_path; }
    const std::string& getQuery() const { return m_query; }
    const std::string& getBody() const { return m_body; }
    const MapType& getHeaders() const { return m_headers; }
    const MapType& getParams() const { return m_params; }
    const MapType& getCookies() const { return m_cookies; }

    void setMethod(HttpMethod v) { m_method = v; }
    void setVersion(uint8_t v) { m_version = v; }
    void setPath(const std::string& v) { m_path = v; }
    void setQuery(const std::string& v) { m_query = v; }
    void setFragment(const std::string& v) { m_fragment = v; }
    void setBody(const std::string& v) { m_body = v; }

    bool isClose() const { return m_close; }
    void setClose(bool v) { m_close = v; }
    bool isWebsocket() const { return m_websocket; }
    void setWebsocket(bool v) { m_websocket = v; }
    
    void setHeaders(const MapType& v) { m_headers = v; }
    void setParams(const MapType& v) { m_params = v; }
    void setCookies(const MapType& v) { m_cookies = v; }
    
    std::string getHeader(const std::string& key, const std::string& def = "") const;
    std::string getParam(const std::string& key, const std::string& def = "");
    std::string getCookie(const std::string& key, const std::string& def = "");
    
    void setHeader(const std::string& key, const std::string& val);
    void setParam(const std::string& key, const std::string& val);
    void setCookie(const std::string& key, const std::string& val);
    
    void delHeader(const std::string& key);
    void delParam(const std::string& key);
    void delCookie(const std::string& key);

    bool hasHeader(const std::string& key, std::string* val = nullptr);
    bool hasParam(const std::string& key, std::string* val = nullptr);
    bool hasCookie(const std::string& key, std::string* val = nullptr);

    template<class T>
    bool checkGetHeadersAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_headers, key, val, def);
    }

    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T())
    {
        return getAs(m_headers, key, def);
    }

    template<class T>
    bool checkGetParamAs(const std::string& key, T& val, const T& def = T())
    {
        initQueryParam();
        initBodyParam();
        return checkGetAs(m_params, key, val, def);
    }

    template<class T>
    T getParamAs(const std::string& key, const T& def = T())
    {
        initQueryParam();
        initBodyParam();
        return getAs(m_params, key, def);
    }
    
    template<class T>
    bool checkGetCookieAs(const std::string& key, T& val, const T& def = T())
    {
        initCookies();
        return checkGetAs(m_cookies, key, val, def);
    }

    template<class T>
    T getCookieAs(const std::string& key, const T& def = T())
    {
        return getAs(m_cookies, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    void init();
    void initParam();
    void initQueryParam();
    void initBodyParam();
    void initCookies();

private:
    /// http方法
    HttpMethod m_method;
    /// http版本
    uint8_t m_version;
    /// 是否自动关闭
    bool m_close;
    /// 是否为websocket
    bool m_websocket;
    
    /// 是否解析完报文的标识，0001b是请求参数、0010b是请求消息体、0100b是请求cookie
    uint8_t m_parserParamFlag;
    /// 请求路径
    std::string m_path;
    /// 请求参数
    std::string m_query;
    /// 请求fragment
    std::string m_fragment;
    /// 请求消息体
    std::string m_body;
    /// 请求头部map
    MapType m_headers;
    /// 请求参数map
    MapType m_params;
    /// 请求cookie map
    MapType m_cookies;
};

/**
 * @brief   http 响应结构
 */
class HttpResponse
{
public:
    typedef std::shared_ptr<HttpResponse> ptr;
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief   构造函数
     *
     * @param   version http版本号
     * @param   close   是否自动关闭
     */
    HttpResponse(uint8_t version = 0x11, bool close = true);
   
    HttpStatus getStatus() const { return m_status; }
    uint8_t getVersion() const { return m_version; }
    const std::string& getBody() const { return m_body; }
    const std::string& getReason() const { return m_reason; }
    const MapType& getHeaders() const { return m_headers; }
    void setStatus(HttpStatus v) { m_status = v; }
    void setVersion(uint8_t v) { m_version = v; }
    void setBody(const std::string& v) { m_body = v; }
    void setReason(const std::string& v) { m_reason = v; }
    void setHeaders(const MapType& v) { m_headers = v; }

    bool isClose() const { return m_close; }
    void setClose(bool v) { m_close = v; }
    bool isWebsocket() const { return m_websocket; }
    void setWebsocket(bool v) { m_websocket = v; }

    std::string getHeader(const std::string& key, const std::string& def = "") const;
    void setHeader(const std::string& key, const std::string& val);
    void delHeader(const std::string& key);

    template<class T>
    bool checkGetHeadersAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(m_headers, key, val, def);
    }

    template<class T>
    T getHeaderAs(const std::string& key, const T& def = T())
    {
        return getAs(m_headers, key, def);
    }

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    void setRedirect(const std::string& uri);
    void setCookie(const std::string& key, const std::string& val, 
                   time_t expired = 0, const std::string& path = "",
                   const std::string& domain = "", bool secure = false);

private:
    /// 响应状态
    HttpStatus m_status;
    /// http版本
    uint8_t m_version;
    /// 是否自动关闭
    bool m_close;
    /// 是否为websocket
    bool m_websocket;
    
    /// 响应消息体
    std::string m_body;
    /// 响应原因
    std::string m_reason;
    /// 响应头部map
    MapType m_headers;
    /// 响应cookie 
    std::vector<std::string> m_cookies;
};

std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);

}   // end namespace http

}   // end namespace sylar

#endif
