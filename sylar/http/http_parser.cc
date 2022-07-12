#include "http_parser.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include <string.h>

namespace sylar
{

namespace http
{
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static sylar::ConfigVar<uint64_t>::ptr g_http_request_buffer_size = 
    sylar::Config::Lookup("http.request.buffer_size", (uint64_t)(4*1024), "http request buffer size");

static sylar::ConfigVar<uint64_t>::ptr g_http_request_max_body_size = 
    sylar::Config::Lookup("http.request.max_body_size", (uint64_t)(64*1024*1024), "http request max body size");

static sylar::ConfigVar<uint64_t>::ptr g_http_response_buffer_size = 
    sylar::Config::Lookup("http.response.buffer_size", (uint64_t)(4*1024), "http response buffer size");

static sylar::ConfigVar<uint64_t>::ptr g_http_response_max_body_size = 
    sylar::Config::Lookup("http.response.max_body_size", (uint64_t)(64*1024*1024), "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

namespace
{
struct _RequestSizeIniter
{
    _RequestSizeIniter()
    {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        g_http_request_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_buffer_size = nv;
        });

        g_http_request_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_max_body_size = nv;
        });

        g_http_response_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_buffer_size = nv;
        });

        g_http_response_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_max_body_size = nv;
        });
    }
};

static _RequestSizeIniter _init;
}

void on_request_method(void* data, const char* at, size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = CharsToHttpMethod(at);

    if(m == HttpMethod::INVALID_METHOD)
    {
        SYLAR_LOG_WARN(g_logger) << "invalid http request method: "
            << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}

void on_request_uri(void* data, const char* at, size_t length)
{
}

void on_request_fragment(void* data, const char* at, size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void* data, const char* at, size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void* data, const char* at, size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void* data, const char* at, size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0)
    {
        v = 0x11;
    }
    else if(strncmp(at, "HTTP/1.0", length) == 0)
    {
        v = 0x10;
    }
    else
    {
        SYLAR_LOG_WARN(g_logger) << "invalid http request version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_request_header_done(void* data, const char* at, size_t length)
{
}

void on_request_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0)
    {
        SYLAR_LOG_WARN(g_logger) << "invalid http request field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser()
    : m_error(0)
{
    m_data.reset(new sylar::http::HttpRequest);
    
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

size_t HttpRequestParser::execute(char* data, size_t len)
{
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    
    // void *memmove(void *str1, const void *str2, size_t n) 从 str2 复制 n 个字符到 str1，
    // 但是在重叠内存块这方面，memmove() 是比 memcpy() 更安全的方法。
    // 如果目标区域和源区域有重叠的话，memmove() 能够保证源串在被覆盖之前将重叠区域的字节拷贝到目标区域中，
    // 复制后源区域的内容会被更改。如果目标区域与源区域没有重叠，则和 memcpy() 函数功能相同。
    
    // 比如len=10字节，offset=3字节，data指向0字节，
    // 则意思就是将后面10-3=7个字节移动到前面去，实现所谓的“移除已解析数据”的功能
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpRequestParser::isFinished()
{
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::hasError()
{
    return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParser::getContentLength()
{
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

uint64_t HttpRequestParser::GetHttpRequestBufferSize()
{
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize()
{
    return s_http_request_max_body_size;
}

void on_response_reason(void* data, const char* at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void* data, const char* at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(status);
}

void on_response_chunk(void* data, const char* at, size_t length)
{
}

void on_response_version(void* data, const char* at, size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0)
    {
        v = 0x11;
    }
    else if(strncmp(at, "HTTP/1.0", length) == 0)
    {
        v = 0x10;
    }
    else
    {
        SYLAR_LOG_WARN(g_logger) << "invalid http response version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_response_header_done(void* data, const char* at, size_t length)
{
}

void on_response_last_chunk(void* data, const char* at, size_t length)
{
}

void on_response_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0)
    {
        SYLAR_LOG_WARN(g_logger) << "invalid http response field length == 0";
        return;
    }
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser()
    : m_error(0)
{
    m_data.reset(new sylar::http::HttpResponse);
    
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

size_t HttpResponseParser::execute(char* data, size_t len, bool chunck)
{
    if(chunck)
    {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpResponseParser::isFinished()
{
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError()
{
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::getContentLength()
{
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize()
{
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()
{
    return s_http_response_max_body_size;
}

}

}
