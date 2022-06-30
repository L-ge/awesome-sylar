/**
 * @filename    iomanager.h
 * @brief   IO协程调度模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-30
 */
#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar
{

/**
 * @brief   基于 epoll 的 IO 协程调度器
 */
class IOManager : public Scheduler, public TimerManager
{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    /**
     * @brief  IO事件，直接继承自 epoll 对事件的定义
     *         只关心读写事件，其他事件也会归类到这两类事件
     */
    enum Event
    {
        NONE    = 0x0,  // 无事件
        READ    = 0x1,  // 读事件(EPOLLIN)
        WRITE   = 0x4,  // 写事件(EPOLLOUT)   
    };

private:
    /**
     * @brief   socket事件上下文
     */
    struct FdContext
    {
        typedef Mutex MutexType;

        /**
         * @brief  事件上下文 
         */
        struct EventContext
        {
            Scheduler* scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        /**
         * @brief   获取对应事件的上下文
         */
        EventContext& getContext(Event event);
        
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        /// 读事件上下文
        EventContext read;
        /// 写事件上下文
        EventContext write;
        /// 描述符
        int fd = 0;
        /// 所含事件
        Event events = NONE;
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true,  const std::string& name = "");
    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);

    /**
     * @brief  判断是否可以停止 
     *
     * @param   timeout 最近要触发的定时器事件间隔
     */
    bool stopping(uint64_t& timeout);

private:
    int m_epfd;
    /// 用于tickele的pipe文件描述符，fd[0]是读端，fd[1]是写端
    int m_tickleFds[2];
    /// 正在等待执行的IO事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    /// socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};

}

#endif
