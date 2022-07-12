# 概述

- 对 http 协议的封装。


# http.h

- http 定义结构体封装模块。
- 一些方法描述和错误码定义是直接拷贝自以下地址：
	` https://github.com/nodejs/http-parser/blob/main/http_parser.h`
- HttpMethod
	- http 方法枚举类。
	- 利用了拷贝回来的宏定义。
- HttpStatus
	- http 状态枚举类。
	- 利用了拷贝回来的宏定义。
- HttpRequest
	- 请求的结构体的封装。
	- 例如 请求方法、请求的 http 版本号、请求路径、请求参数、请求消息体、cookie 等等。
- HttpResponse
	- 响应的结构体的封装。
	- 例如 响应状态、响应的 http 版本、响应消息体、响应原因 等等。


# http_parser.h

- HttpRequestParser
	- http请求解析类。
- HttpResponseParser
	- http响应解析类。
- 这两个类主要是设置一些回调函数，然后通过这些回调函数来设置某些属性。


# 其他说明

- http_parser.h 模块利用了 Ragel 来解析 http 协议报文。
	- Ragel 是个有限状态机编译器，它将基于正则表达式的状态机编译成传统语言（C，C++，D，Java，Ruby 等）的解析器。Ragel 不仅仅可以用来解析字节流，它实际上可以解析任何可以用正则表达式表达出来的内容。而且可以很方便的将解析代码嵌入到传统语言中。
- http_parser.h 模块利用了别人写好的按照 ragel 提供的语法与关键字编写的 .rl 文件，从中拷贝出 3 个头文件，2 个 rl 文件。其中 .c 文件是通过 rl 文件生成的。
	- 具体地址如下：
		`https://github.com/mongrel2/mongrel2/tree/master/src/http11`
	- 具体生成 .cc 文件的指令如下：
		```shell
		$ ragel -v			// 获取版本号
		$ ragel -help		// 获取帮助文档
		
		$ ragel -G2 -C http11_parser.rl -o http11_parser.cc
		$ ragel -G2 -C httpclient_parser.rl -o httpclient_parser.cc
		$ mv http11_parser.cc http11_parser.rl.cc			  // 与自己写的代码区分开
		$ mv httpclient_parser.cc httpclient_parser.rl.cc     // 与自己写的代码区分开
		```
	- 注意，sylar 对拷贝回来的几个文件做了一些修改，使其编译通过和支持“分段解析”的功能。
- http.h 中 HttpRequest 和 HttpResponse 的 dump 方法里面组装出来的字符串其实就是 http 报文了。
