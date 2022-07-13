# 概述

- TCP 服务器的封装。


# TcpServerConf

- TcpServer 配置结构体。
- 两个模板偏特化：
	- template<> class LexicalCast<std::string, TcpServerConf>
	- template<> class LexicalCast<TcpServerConf, std::string>


# TcpServer

- 继承自 std::enable_shared_from_this<TcpServer> 和 sylar::Noncopyable。
-  TcpServer 类支持同时绑定多个地址进行监听，只需要在绑定时传入地址数组即可。有两个重载的 bind 方法。
-  TcpServer 还可以分别指定接收客户端和处理客户端的协程调度器。具体是 m_acceptWorker 和 m_ioWorker。


# 其他说明

- TcpServer 类采用了模板模式（Template Pattern）的设计模式，它的 HandleClient 是交由继承类来实现的。使用 TcpServer 时，必须从 TcpServer 派生一个新类，并重新实现子类的 handleClient 操作。
- 在模板模式（Template Pattern）中，一个抽象类公开定义了执行它的方法的方式/模板。它的子类可以按需要重写方法实现，但调用将以抽象类中定义的方式进行。这种类型的设计模式属于行为型模式。


# 待完善

- 暂未调试支持 SSL。
