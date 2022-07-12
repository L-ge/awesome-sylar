/**
 * @filename    http_parser.h
 * @brief   http 协议解析封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-11
 */
#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar
{

namespace http
{

/**
 * @brief   http请求解析类
 */
class HttpRequestParser
{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;

    HttpRequestParser();

    /**
     * @brief  解析协议 
     *
     * @param   data    协议文本缓存
     * @param   len     协议文本缓存长度
     *
     * @return  返回实际解析的长度，并且将已解析的数据移除
     */
    size_t execute(char* data, size_t len);
    
    /**
     * @brief   是否解析完成
     */
    int isFinished();
    
    /**
     * @brief   是否有错误
     */
    int hasError();
    
    HttpRequest::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }
    uint64_t getContentLength();
    const http_parser& getParser() const { return m_parser; }

public:
    /**
     * @brief   获取HttpRequest协议解析的缓存大小
     */
    static uint64_t GetHttpRequestBufferSize();
    
    /**
     * @brief   获取HTTPRequest协议的最大消息体大小
     */
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /// 错误码
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

/**
 * @brief   http响应解析类
 */
class HttpResponseParser
{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    HttpResponseParser();

    /**
     * @brief   解析http响应协议
     *
     * @param   data    协议数据缓存
     * @param   len     协议数据缓存大小
     * @param   chunck  是否在解析chunck
     *          Chunk就是将大文件分成块，一个块对应着一个Http请求，然后会对每个Http进行编号，然后在接收方重组。
     *          正常的Http请求都是客户端请求，服务器返回然后就结束了。
     *          而Chunk不会，是会一直等待服务器多次发送数据，发送数据完成后才会结束。
     *          通过Header中的Transfer-Encoding = Chunked判断一个http是不是chunk。
     * @return  返回实际解析的长度，并且移除已解析的数据
     */
    size_t execute(char* data, size_t len, bool chunck);
    int isFinished();
    int hasError();
    HttpResponse::ptr getData() const { return m_data; }
    void setError(int v) { m_error = v; }
    uint64_t getContentLength();
    const httpclient_parser& getParser() const { return m_parser; }

public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();

private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    /// 错误码
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

}   // end namespace http

}   // end namespace sylar


#endif
