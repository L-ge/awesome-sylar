# 概述

- 流结构，提供字节流读写接口。
- 所有的流结构都继承自抽象类 Stream，Stream 类规定了一个流必须具备 read / write 接口和 readFixSize / writeFixSize 接口，继承自 Stream 的类必须实现这些接口。


# Stream

- 流接口的封装类。
- 虚基类。
- 含有纯虚方法 read / write 和 readFixSize / writeFixSize。


# SocketStream

- Socket 流式接口的封装。
- 继承自 Stream。
- 套接字流结构，将套接字封装成流结构，以支持 Stream 接口规范。
- 支持套接字关闭操作以及获取本地/远端地址的操作。
