# 概述

- HTTPServlet 模块。
- 提供 HTTP 请求路径到处理类的映射，用于规范化的 HTTP 消息处理流程。
- HTTP Servlet 包括两部分，第一部分是 Servlet 对象，每个 Servlet 对象表示一种处理 HTTP 消息的方法；第二部分是 ServletDispatch，它包含一个请求路径到 Servlet 对象的映射，用于指定一个请求路径该用哪个 Servlet 来处理。

# Servlet

- 纯虚基类，子类必须重写其虚方法 handleClient。


# FunctionServlet

- 函数式 Servlet。
- 继承自 Servlet。
- 内含回调函数，其 handleClient 方法其实是直接调用回调函数。


# NotFoundServlet

- ServletDispatch 默认的 Servlet，404 那种。
- 继承自 Servlet。


# IServletCreator

- 纯虚基类，子类必须重写其 get() 和 getName() 方法。


# HoldServletCreator

- 继承自 IServletCreator。


# ServletCreator

- 继承自 IServletCreator。
- 模板类，根据类型创建 Servlet。


# ServletDispatch

- Servlet 分发器。
- 继承自 Servlet。
- 内含精准匹配的 Servlet map 和 模糊匹配的 Servlet vector，并提供增、删、查方法。
