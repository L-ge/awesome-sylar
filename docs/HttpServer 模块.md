# 概述

- Http 服务器的封装。


# HttpServer

- HttpServer 封装类。
- 继承自 TcpServer。
- 内含 Servlet 分发器（可设置、可获取）。
- 重写了 TcpServer 的 handleClient 方法。


# 接收请求和发送响应的数据流转

```C++
new sylar::http::HttpServer
		↓
HttpServer::bind (socket bind 和 listen)
		↓
HttpServer::start (放入调度器 accept)
		↓
收到请求：HttpServer::handleClient，创建 HttpSession
		↓
在HttpSession::recvRequest() 读取报文(其实是从 SocketStream 读的)
		↓
用 HttpRequestParser 解析报文并组装得到 HttpRequest
		↓
创建 HttpResponse，并把 HttpRequest、HttpResponse、HttpSession 一并交给 ServletDispatch 去处理

ServletDispatch 根据HttpRequest 的 path 匹配最佳的 Servlet 进行处理
		↓
Servlet 的处理是设置 HttpResponse 的一些内容(也就是响应报文))
		↓
ServletDispatch 处理完，HttpSession 将 HttpResponse 发出去(数据传给 Stream- > SocketStream，最后通过 socket send 出去，socket 的 send 是 hook 过的)
```
