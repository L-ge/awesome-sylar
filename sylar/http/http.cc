#include "http.h"
#include "sylar/util.h"

namespace sylar
{

namespace http
{

HttpMethod StringToHttpMethod(const std::string& m)
{
#define XX(num, name, string) \
    if(strcmp(#string, m.c_str()) == 0) \
    { \
        return HttpMethod::name; \
    } \
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

HttpMethod CharsToHttpMethod(const char* m)
{
#define XX(num, name, string) \
    if(strncmp(#string, m, strlen(#string)) == 0) \
    { \
        return HttpMethod::name; \
    } \
    
    HTTP_METHOD_MAP(XX);
#undef XX
    return HttpMethod::INVALID_METHOD;
}

static const char* s_method_string[] =
{
#define XX(num, name, string) #string,
    HTTP_METHOD_MAP(XX)
#undef XX
};

const char* HttpMethodToString(const HttpMethod& m)
{
    uint32_t idx = (uint32_t)m;
    if(idx >= (sizeof(s_method_string) / sizeof(s_method_string[0])))
    {
        return "<unknown>";
    }
    return s_method_string[idx];
}

const char* HttpStatusTostring(const HttpStatus& s)
{
    switch(s)
    {
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
        HTTP_STATUS_MAP(XX);
#undef XX
        default:
            return "<unknown>";
    }
}

bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const
{
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    : m_method(HttpMethod::GET)
    , m_version(version)
    , m_close(close)
    , m_websocket(false)
    , m_parserParamFlag(0)
    , m_path("/")
{
}

std::shared_ptr<HttpResponse> HttpRequest::createResponse()
{
    HttpResponse::ptr rsp(new HttpResponse(getVersion(), isClose()));
    return rsp;
}

std::string HttpRequest::getHeader(const std::string& key, const std::string& def) const
{
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

std::string HttpRequest::getParam(const std::string& key, const std::string& def)
{
    initQueryParam();
    initBodyParam();
    auto it = m_params.find(key);
    return it == m_params.end() ? def : it->second;
}

std::string HttpRequest::getCookie(const std::string& key, const std::string& def)
{
    initCookies();
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? def : it->second;
}

void HttpRequest::setHeader(const std::string& key, const std::string& val)
{
    m_headers[key] = val;
}

void HttpRequest::setParam(const std::string& key, const std::string& val)
{
    m_params[key] = val;
}

void HttpRequest::setCookie(const std::string& key, const std::string& val)
{
    m_cookies[key] = val;
}

void HttpRequest::delHeader(const std::string& key)
{
    m_headers.erase(key);
}

void HttpRequest::delParam(const std::string& key)
{
    m_params.erase(key);
}

void HttpRequest::delCookie(const std::string& key)
{
    m_cookies.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key, std::string* val)
{
    auto it = m_headers.find(key);
    if(it == m_headers.end())
    {
        return false;
    }
    if(val)
    {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasParam(const std::string& key, std::string* val)
{
    initQueryParam();
    initBodyParam();
    auto it = m_params.find(key);
    if(it == m_params.end())
    {
        return false;
    }
    if(val)
    {
        *val = it->second;
    }
    return true;
}

bool HttpRequest::hasCookie(const std::string& key, std::string* val)
{
    initCookies();
    auto it = m_cookies.find(key);
    if(it == m_cookies.end())
    {
        return false;
    }
    if(val)
    {
        *val = it->second;
    }
    return true;
}

std::ostream& HttpRequest::dump(std::ostream& os) const
{
    os << HttpMethodToString(m_method) << " "
       << m_path
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << " HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    if(!m_websocket)
    {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }

    for(auto& i : m_headers)
    {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)
        {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    if(!m_body.empty())
    {
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    }
    else
    {
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

void HttpRequest::init()
{
    std::string conn = getHeader("connection");
    if(!conn.empty())
    {
        if(strcasecmp(conn.c_str(), "keep-alive") == 0)
        {
            m_close = false;
        }
        else
        {
            m_close = true;
        }
    }
}

void HttpRequest::initParam()
{
    initQueryParam();
    initBodyParam();
    initCookies();
}

void HttpRequest::initQueryParam()
{
    if(m_parserParamFlag & 0x1)     // 已经解析过了，直接return
    {
        return;
    }

#define PARSE_PARAM(str, m, flag, trim) \
    size_t pos = 0; \
    do \
    { \
        size_t last = pos; \
        pos = str.find('=', pos); \
        if(pos == std::string::npos) \
        { \
            break; \
        } \
        size_t key = pos; \
        pos = str.find(flag, pos); \
        m.insert(std::make_pair(trim(str.substr(last, key-last)), \
                    sylar::StringUtil::UrlDecode(str.substr(key+1, pos-key-1)))); \
        if(pos == std::string::npos) \
        { \
            break; \
        } \
        ++pos; \
    } while(true);

    PARSE_PARAM(m_query, m_params, '&',);
    m_parserParamFlag |= 0x1;
}

void HttpRequest::initBodyParam()
{
    if(m_parserParamFlag & 0x2)
    {
        return;
    }

    std::string content_type = getHeader("content-type");

    // form的enctype属性为编码方式，常用有两种：application/x-www-form-urlencoded和multipart/form-data，默认为application/x-www-form-urlencoded。
    // 1.x-www-form-urlencoded
    // 当action为get时候，浏览器用x-www-form-urlencoded的编码方式把form数据转换成一个字串（name1=value1&name2=value2…），然后把这个字串append到url后面，用?分割，加载这个新的url。
    //2.multipart/form-data
    //当action为post时候，浏览器把form数据封装到http body中，然后发送到server。 如果没有type=file的控件，用默认的application/x-www-form-urlencoded就可以了。 但是如果有type=file的话，就要用到multipart/form-data了。浏览器会把整个表单以控件为单位分割，并为每个部分加上Content-Disposition(form-data或者file),Content-Type(默认为text/plain),name(控件name)等信息，并加上分割符(boundary)。
    
    // 请求头Content-Type为application/x-www-form-urlencoded，意思是这个url是被编码过的。
    // 如果请求端未执行编码操作，那么响应端将获取不到传过来的参数
    if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr)
    {
        m_parserParamFlag |= 0x2;
        return;
    }
    PARSE_PARAM(m_body, m_params, '&',);
    m_parserParamFlag |= 0x2;
}

void HttpRequest::initCookies()
{
    if(m_parserParamFlag & 0x4)
    {
        return;
    }
    std::string cookie = getHeader("cookie");
    if(cookie.empty())
    {
        m_parserParamFlag |= 0x4;
        return;
    }
    PARSE_PARAM(cookie, m_cookies, ';', sylar::StringUtil::Trim);
    m_parserParamFlag |= 0x4;
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    : m_status(HttpStatus::OK)
    , m_version(version)
    , m_close(close)
    , m_websocket(false)
{
}

std::string HttpResponse::getHeader(const std::string& key, const std::string& def) const
{
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}

void HttpResponse::setHeader(const std::string& key, const std::string& val)
{
    m_headers[key] = val;
}

void HttpResponse::delHeader(const std::string& key)
{
    m_headers.erase(key);
}

std::ostream& HttpResponse::dump(std::ostream& os) const
{
    os << "HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << " "
       << (uint32_t)m_status
       << " "
       << (m_reason.empty() ? HttpStatusTostring(m_status) : m_reason)
       << "\r\n";

    for(auto& i : m_headers)
    {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0)
        {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    for(auto& i : m_cookies)
    {
        os << "Set-Cookie: " << i << "\r\n";
    }

    if(!m_websocket)
    {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if(!m_body.empty())
    {
        os << "content-length: " << m_body.size() << "\r\n\r\n" << m_body;
    }
    else
    {
        os << "\r\n";
    }
    return os;
}

std::string HttpResponse::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

void HttpResponse::setRedirect(const std::string& uri)
{
    // Location 首部指定的是需要将页面重新定向至的地址。一般在响应码为 3xx 的响应中才会有意义。
    // location属性表示该网页的跳转网址，也被称为重定向网址。当网页中包含该属性后，浏览器会自动从当前网页跳转到location中指定的网址。
    m_status = HttpStatus::FOUND;       // FOUND是302
    setHeader("Location", uri);
}

void HttpResponse::setCookie(const std::string& key, const std::string& val, 
                   time_t expired, const std::string& path,
                   const std::string& domain, bool secure)
{
    std::stringstream ss;
    ss << key << "=" << val;
    if(expired > 0)     // 设置Cookie的生存期
    {
        ss << ";expires=" 
           << sylar::Time2Str(expired, "%a, %d %b %Y:%M:%S")
           << " GMT";
    }
    if(!domain.empty()) // 指定了可以访问该Cookie的Web站点或域
    {
        ss << ";domain=" << domain;
    }
    if(!path.empty())   // 定义了Web站点上可以访问该Cookie的目录
    {
        ss << ";path=" << path;
    }
    if(secure)          // 指定是否使用HTTPS安全协议发送Cookie
    {
        ss << ";secure";
    }
    m_cookies.push_back(ss.str());
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req)
{
    return req.dump(os);
}

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp)
{
    return rsp.dump(os);
}

}

}
