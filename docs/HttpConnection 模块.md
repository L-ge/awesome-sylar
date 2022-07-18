# 概述

- Http 客户端的封装。


# http_connection.h

- HttpResult
	- 内含枚举类——错误码定义 Error。
	- 对响应结果的封装。
- HttpConnection
	- 继承自 SocketStream。
- HttpConnectionPool
	- HttpConnectionPool 针对的是长连接的场景。
	- 留意里面的 ReleasePtr 方法，算是一种优化。


# uri.h 和 uri.rl

- url 是 uri 的一种实例。
- uri 定义的文档参考如下：
	`https://www.ietf.org/rfc/rfc3986.txt`
- uri.rl 仿照上面的文档写的，最后的 uri 用 ragel 进行解析。
- 在 uri.rl 中，在那些解析部分，其实就是通过像下面这样去解析并赋值到 uri 对象中。
	```
	action save_scheme
	{
		uri->setScheme(std::string(mark, fpc - mark));
		mark = NULL;
	}
	```
- 命令去生成 .cc 文件：
	```
	$ ragel -G2 -C uri.rl -o uri.cc
	mv uri.cc uri.rl.cc
	```
