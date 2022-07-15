#include "http_session.h"
#include "http_parser.h"

namespace sylar
{

namespace http
{

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner)
{
}

HttpRequest::ptr HttpSession::recvRequest()
{
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    std::shared_ptr<char> buffer(new char[buff_size], [](char* ptr){ delete[] ptr; });  // 自定义删除函数
    char* data = buffer.get();      // 虽然用了智能指针，但是使用的时候还是直接用裸指针
    int offset = 0;     // 偏移量，尚未解析完的数据
    // 先去理解parser->execute里面的memmove方法，再看这个do-while循环会比较容易理解
    do
    {
        int len = read(data + offset, buff_size - offset);  // 有offset个字节的数据尚未解析，因此偏移量是offset
        if(len <= 0)
        {
            close();
            return nullptr;
        }
        len += offset;                  // 加上上一次没解析完的数据，这次再一起解析
        size_t nparse = parser->execute(data, len); // 这里面有memmove的动作，所以是从data开始解析就行
        if(parser->hasError())
        {
            close();
            return nullptr;
        }
        offset = len - nparse;          // 尚未解析完成的数据
        if(offset == (int)buff_size)    // 尚未解析完的数据等于原始数据，相当于没解析成功一点东西，即失败
        {
            close();
            return nullptr;
        }
        if(parser->isFinished())
        {
            break;
        }
    } while(true);

    int64_t length = parser->getContentLength();    // 消息主体的长度(不包括http首部)
    if(length > 0)
    {
        std::string body;
        body.resize(length);
        
        int len = 0;
        if(length >= offset)
        {
            memcpy(&body[0], data, offset);
            len = offset;
        }
        else
        {
            memcpy(&body[0], data, length);
            len = length;
        }

        length -= offset;   // 减掉已经copy到body的长度
        if(length > 0)
        {
            if(readFixSize(&body[len], length) <= 0)
            {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    parser->getData()->init();  // 里面主要是初始化是否长连接
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp)
{
    std::stringstream ss;
    ss << *rsp;             // 重载了<<运算符，输出的其实就是一个http报文了
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}

}
