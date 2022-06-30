# 概述

- 基于 epoll 的IO协程调度器。
- 支持读写事件。
- IO协程调度器还解决了上一章协程调度器在 idle 状态下忙等待导致 CPU 占用率高的问题。IO协程调度器使用一对管道 fd 来 tickle 调度协程，当调度器空闲时，idle 协程通过 epoll_wait 阻塞在管道的读描述符上，等管道的可读事件。添加新任务时，tickle 方法写管道，idle 协程检测到管道可读后退出，调度器执行调度。
- IO协程调度支持为描述符注册可读和可写事件的回调函数，当描述符可读或可写时，执行对应的回调函数（包装在 FdContext::EventContext 里面）。


# IOManager

- IO协程调度器类。
- 继承自协程调度器。
- 重载了 Scheduler 的 tickle 和 idle 方法。


# 其他说明

- 对每个 fd，sylar 支持两类事件，一类是可读事件（对应 EPOLLIN），一类是可写事件（对应EPOLLOUT），sylar的事件枚举值直接继承自 epoll。
- 当然 epoll 本身除了支持了 EPOLLIN 和 EPOLLOUT 两类事件外，还支持其他事件，比如 EPOLLRDHUP, EPOLLERR, EPOLLHUP 等，对于这些事件，sylar 的做法是将其进行归类，分别对应到 EPOLLIN 和 EPOLLOUT 中，也就是所有的事件都可以表示为可读或可写事件，甚至有的事件还可以同时表示可读及可写事件，比如 EPOLLERR 事件发生时，fd 将同时触发可读和可写事件。
- 在执行 epoll_ctl 时通过 epoll_event 的私有数据指针 data.ptr 来保存 FdContext 结构体信息，其中 FdContext 结构体信息包括描述符 fd、所含事件 events、回调函数 EventContext。
- IO协程调度器在 idle 时会 epoll_wait 所有注册的fd，如果有 fd 满足条件，epoll_wait 返回，从私有数据中拿到 fd 的上下文信息（也就是 data.ptr 里面存放的 FdContext），并且执行其中的回调函数（实际是 idle 协程只负责收集所有已触发的 fd 的回调函数并将其加入调度器的任务队列，真正的执行时机是 idle 协程退出后，调度器在下一轮调度时执行）。
- epoll 是线程安全的，即使调度器有多个调度线程，它们也可以共用同一个 epoll 实例，而不用担心互斥。由于空闲时所有线程都阻塞的 epoll_wait 上，所以也不用担心 CPU 占用问题。
- addEvent 是一次性的，比如说，注册了一个读事件，当 fd 可读时会触发该事件，但触发完之后，这次注册的事件就失效了，后面 fd 再次可读时，并不会继续执行该事件回调，如果要持续触发事件的回调，那每次事件处理完都要手动再 addEvent。这样在应对 fd 的 WRITE 事件时会比较好处理，因为 fd 可写是常态，如果注册一次就一直有效，那么可写事件就必须在执行完之后就删除掉。
