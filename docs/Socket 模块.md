# 概述

- 套接字类，表示一个套接字对象。
- 其实就是在操作系统的 API 上再封装了一层。


# Socket

- 继承自 std::enable_shared_from_this<Socket> 和 sylar::Noncopyable。
- 含有以下属性：
	- 文件描述符 m_sock。
	- 地址类型（AF_INET，AF_INET6 等）m_family。
	- 套接字类型（SOCK_STREAM，SOCK_DGRAM 等）m_tyoe。
	- 协议类型（这项其实可以忽略）m_protocol。
	- 是否连接（针对 TCP 套接字；如果是 UDP 套接字，则默认已连接）m_isConnected。
	- 本地地址 m_localAddress。
	- 远端地址 m_remoteAddress。
- 含有如下方法：
	- 创建各种类型的套接字对象的方法（TCP 套接字，UDP 套接字，Unix 域套接字）。
	- 设置套接字选项，比如超时参数。
	- bind / connect / listen 方法，实现绑定地址、发起连接、发起监听功能。
	- accept 方法，返回连入的套接字对象。
	- 发送和接收数据的方法。
	- 获取本地地址和远端地址的方法。
	- 获取套接字类型、地址类型、协议类型的方法。
	- 取消套接字读、写事件的方法。


# SSLSocket

- 继承自 Socket。
- 基于 openssl。


# 其他说明

- UnixTCPSocket 在 bind 函数里面 connect。
