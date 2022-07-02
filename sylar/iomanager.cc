#include "iomanager.h"
#include "macro.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

namespace sylar
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

enum EpollCtlOp
{};

static std::ostream& operator<<(std::ostream& os, const EpollCtlOp& op)
{
    switch((int)op)
    {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<<(std::ostream& os, EPOLL_EVENTS events)
{
    if(!events)
    {
        return os << "0";
    }

    bool first = true;
#define XX(E) \
    if(events & E) \
    { \
        if(!first) \
        { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event)
{
    switch(event)
    {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event)
{
    SYLAR_ASSERT(events & event);           // 触发的事件必须是先存在的
    events = (Event)(events & ~event);      // 去掉要触发的事件
    EventContext& ctx = getContext(event);  // 拿到要触发的事件上下文，将它放入调度器里面去
    if(ctx.cb)
    {
        ctx.scheduler->schedule(&ctx.cb);
    }
    else
    {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller,  const std::string& name)
    : Scheduler(threads, use_caller, name)
{
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    // 创建pipe，获取m_tickeleFds[2]，其中[0]是管道的读端，[1]是管道的写端
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    // 注册pipe的读事件，ET模式。通过data.fd保存文件描述符
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    // 设置为非阻塞，配合ET模式使用
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    contextResize(32);

    // 创建即可调度协程
    start();
}

IOManager::~IOManager()
{
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i=0; i<m_fdContexts.size(); ++i)
    {
        if(m_fdContexts[i])
        {
            delete m_fdContexts[i];
            m_fdContexts[i] = nullptr;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd)
    {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }
    else
    {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 同一个fd不允许重复添加相同的事件
    if(SYLAR_UNLIKELY(fd_ctx->events & event))
    {
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
            << " event=" << (EPOLL_EVENTS)event
            << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLIN | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt)
    {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++m_pendingEventCount;      // 待执行IO事件数加1

    fd_ctx->events = (Event)(fd_ctx->events | event);

    // 找到这个fd的event事件对应的EventContext，对其中的scheduler、cb或fiber进行赋值
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler
              && !event_ctx.fiber
              && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb)
    {
        event_ctx.cb.swap(cb);
    }
    else
    {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                "state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(SYLAR_UNLIKELY(!(fd_ctx->events & event)))
    {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt)
    {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;      // 待执行事件减1

    // 重置该fd对应的event事件上下文
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

/**
 * @brief  取消事件：如果该事件被注册过回调，那就触发一次回调事件 
 */
bool IOManager::cancelEvent(int fd, Event event)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(SYLAR_UNLIKELY(!(fd_ctx->events & event)))
    {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt)
    {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 触发一次事件
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd)
{
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd)
    {
        return false;
    }

    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events)
    {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt)
    {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    // 触发全部已注册的事件
    if(fd_ctx->events & READ)
    {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE)
    {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

/**
 * @brief   通知调度器有任务要调度
 *          写pipe让idle协程从epoll_wait退出，
 *          等idle协程yield之后，Scheduler::run就可以调度其他任务了
 */
void IOManager::tickle()
{
    // 如果当前没有空闲的调度线程，则直接return掉
    if(!hasIdleThreads())
    {
        return;
    }

    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping()
{
    uint64_t timeout = 0;
    return stopping(timeout);
}

/**
 * @brief   idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
 *
 */
void IOManager::idle()
{
    SYLAR_LOG_DEBUG(g_logger) << "idle";
    // 一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮epoll_wait继续处理
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr)    // 自定义删除函数
    {
        delete[] ptr;
    });

    while(true)
    {
        uint64_t next_timeout = 0;
        if(SYLAR_UNLIKELY(stopping(next_timeout)))
        {
            SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }

        int rt = 0;
        do
        {
            static const int MAX_TIMEOUT = 3000;
            // 选择小的那个时间
            if(next_timeout != ~0ull)
            {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            }
            else
            {
                next_timeout = MAX_TIMEOUT;
            }

            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if(rt < 0 && errno == EINTR)
            {}
            else
            {
                break;
            }
        }while(true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);     // 拿到所有到期的定时任务
        if(!cbs.empty())
        {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(int i=0; i<rt; ++i)
        {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0])
            {
                // tickeleFd[0]用于通知协程调度，
                // 这时只需要把管道里的内容读完即可，
                // 本轮idle结束后，Scheduler::run会重新执行协程调度
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lk(fd_ctx->mutex);
            // EPOLLERR 出错，比如写读端以及关闭的pipe
            // EPOLLHUP 套接字对端关闭
            // 出现这两种事件时，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
            if(event.events & (EPOLLERR | EPOLLHUP))
            {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN)
            {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT)
            {
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE)  // 没有发生所存的事件，则continue
            {
                continue;
            }

            // 剔除已经发生的事件，将剩下的事件重新加入epoll_wait
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2)
            {
                SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            // 处理已经发送的事件，也就是让调度器调度指定的函数或协程
            if(real_events & READ)
            {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE)
            {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        // 一旦处理完所有的事件，idle协程切出，这样就可以让调度协程Scheduler::run重新检查是否有新任务要调度
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

/**
 * @brief   有定时事件插入到定时器的最前面，则马上tickle一下
 */
void IOManager::onTimerInsertedAtFront()
{
    tickle();
}

void IOManager::contextResize(size_t size)
{
    m_fdContexts.resize(size);

    for(size_t i=0; i<m_fdContexts.size(); ++i)
    {
        if(!m_fdContexts[i])
        {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();           // 获取下一个定时器的超时时间
    return timeout == ~0ull
        && m_pendingEventCount == 0     // 所有IO事件都完成调度
        && Scheduler::stopping();
}

}
