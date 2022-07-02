#include "hook.h"
#include "log.h"
#include "config.h"
#include "macro.h"
#include "fiber.h"
#include "iomanager.h"
#include "fdmanager.h"
#include <dlfcn.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar
{

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;     // 当前线程是否 hook 的标志

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init()
{
    static bool is_inited = false;
    if(is_inited)
    {
        return;
    }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    // 这里宏展开之后就是，以name为sleep举例：
    // sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value)
        {
            SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                     << old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable()
{
    return t_hook_enable;
}

void set_hook_enable(bool flag)
{
    t_hook_enable = flag;
}

}  // namespace sylar end

struct timer_info
{
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name, 
                     uint32_t event, int timeout_so, Args&&... args)
{
    if(!sylar::t_hook_enable)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }

    // 如果不是 socket fd 或是用户显式设置过非阻塞模式，那么就不需要 hook 了。
    if(!ctx->isSocket() || ctx->getUserNonblock())
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so); // 得到该类型的超时时间
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(n == -1 && errno == EINTR)    // 被中断
    {
        n = fun(fd, std::forward<Args>(args)...);
    }

    if(n == -1 && errno == EAGAIN)      // 再次尝试（fd是非阻塞的）
    {
        sylar::IOManager* iom = sylar::IOManager::GetThis();
        sylar::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo); // 用于条件定时器

        if(to != (uint64_t)-1)          // 超时时间有效
        {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
            {
                auto t = winfo.lock();
                if(!t || t->cancelled)
                {
                    return;
                }
                // 设置超时标志，并触发一次对应的事件
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (sylar::IOManager::Event)(event));
            }, winfo);
        }

        int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
        if(SYLAR_UNLIKELY(rt))
        {
            SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer)
            {
                timer->cancel();
            }
            return -1;
        }
        else       // 添加事件成功后，当前协程让出CPU
        {
            sylar::Fiber::YieldToHold();
            if(timer)
            {
                timer->cancel();
            }

            // 协程从上面的 YieldHold 点返回，返回之后通过超时标志设置 errno 并返回 -1。
            if(tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;     // 没有超时，则继续再去读写（EAGAIN才会进入这里面）
        }
    }
    
    return n;
}

extern "C"
{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds)
{
    if(!sylar::t_hook_enable)
    {
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind(
                (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, 
                iom, fiber, -1));
    sylar::Fiber::YieldToHold();    // 添加定时器后，当前协程让出CPU，等它从这个点返回的时候，就是超时到了的时候。
    return 0;
}

int usleep(useconds_t usec)
{
    if(!sylar::t_hook_enable)
    {
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind(
                (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, 
                iom, fiber, -1));
    sylar::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem)
{
    if(!sylar::t_hook_enable)
    {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind(
                (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule, 
                iom, fiber, -1));
    sylar::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol)
{
    if(!sylar::t_hook_enable) 
    {
        return socket(domain, type, protocol);
    }
    
    int fd = socket_f(domain, type, protocol);
    if(fd == -1)
    {
        return fd;
    }
    sylar::FdMgr::GetInstance()->get(fd, true); // 如果文件描述符管理器里面没有，则自动创建
    return fd;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen, sylar::s_connect_timeout);
}

int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
{
    int fd = do_io(s, accept_f, "accept", sylar::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0)
    {
        sylar::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void* buf, size_t count)
{
    return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
{
    return do_io(fd, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags)
{
    return do_io(sockfd, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return do_io(sockfd, recvfrom_f, "recvfrom", sylar::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
{
    return do_io(sockfd, recvmsg_f, "recvmsg", sylar::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void* buf, size_t count)
{
    return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
{
    return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void* msg, size_t len, int flags)
{
    return do_io(s, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
{
    return do_io(s, sendto_f, "sendto", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr* msg, int flags)
{
    return do_io(s, sendmsg_f, "sendmsg", sylar::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd)
{
    if(!sylar::t_hook_enable)
    {
        return close_f(fd);
    }

    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(ctx)
    {
        auto iom = sylar::IOManager::GetThis();
        if(iom)
        {
            iom->cancelAll(fd); // 此时会让 fd 的读写事件回调都执行一次
        }
        sylar::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ )
{
    va_list va;
    va_start(va, cmd);
    switch(cmd)
    {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return fcntl_f(fd, cmd, arg);
                }

                ctx->setUserNonblock(arg & O_NONBLOCK); // 用户主动设置非阻塞
                
                if(ctx->getSysNonblock())
                {
                    arg |= O_NONBLOCK;
                }
                else
                {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket())
                {
                    return arg;
                }

                // 用户得到的是否阻塞，是它曾经主动设置过的
                if(ctx->getUserNonblock())
                {
                    return arg |= O_NONBLOCK;
                }
                else
                {
                    return arg &= ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...)
{
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request)      // 用户主动设置是否非阻塞
    {
        bool user_nonblock = !!*(int*)arg;
        sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket())
        {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
    if(!sylar::t_hook_enable)
    {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    if(level == SOL_SOCKET)
    {
        // 用户设置的超时时间要同步给文件描述符管理器
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
        {
            sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(sockfd);
            if(ctx)
            {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
{
    if(!sylar::t_hook_enable)
    {
        return connect_f(fd, addr, addrlen);
    }

    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose())
    {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket())        // 判断是否为套接字
    {
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock())  // 是否被用户显式设置了非阻塞模式
    {
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);   // 套接字是非阻塞的，因此会直接返回EINPROGRESS
    if(n == 0)
    {
        return 0;
    }
    else if(n != -1 || errno != EINPROGRESS)
    {
        return n;
    }

    sylar::IOManager* iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1)  // 传入的超时参数有效
    {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
                {
                    auto t = winfo.lock();
                    if(!t || t->cancelled)
                    {
                        return;
                    }
                    // 设置超时标志，并触发一次写事件
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, sylar::IOManager::WRITE);
                }, winfo);
    }

    int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
    if(rt == 0)
    {
        sylar::Fiber::YieldToHold();    // 成功添加写事件之后就让出，等待写事件触发再往下执行
        // 等待超时或套接字可写，这里是未超时，套接字就可写了，那么直接取消定时器。
        // 就算超时了，也是取消定时器。
        if(timer)
        {
            timer->cancel();
        }

        // 等待超时或套接字可写，如果先超时，则条件变量 winfo 仍然有效，
        // 通过 winfo 拿到 t 来设置超时标志并触发WRITE事件（上面的条件定时器），
        // 协程从上面的 YieldHold 点返回，返回之后通过超时标志设置 errno 并返回 -1。
        if(tinfo->cancelled)
        {
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else
    {
        if(timer)
        {
            timer->cancel();
        }
        SYLAR_LOG_ERROR(g_logger) << "connect addEvent (" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) // 获取socket的错误信息
    {
        return -1;
    }
    if(!error)
    {
        return 0;
    }
    else
    {
        errno = error;
        return -1;
    }
}

}

