/**
 * @filename    scheduler.h
 * @brief   协程调度模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-29
 */
#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include "fiber.h"
#include "thread.h"

namespace sylar
{

/**
 * @brief   协程调度器
 */
class Scheduler
{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /**
     * @brief   构造函数
     *
     * @param   threads     线程数量
     * @param   use_caller  是否使用当前调用线程
     * @param   name        协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }
    
    static Scheduler* GetThis();

    /**
     * @brief   返回当前协程调度器的调度协程(不一定是主协程)
     */
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lk(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if(need_tickle)
        {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lk(m_mutex);
            while(begin != end)
            {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }

        if(need_tickle)
        {
            tickle();
        }
    }

    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);

protected:
    /**
     * @brief  通知协程调度器有任务了 
     */
    virtual void tickle();

    void run();

    virtual bool stopping();

    virtual void idle();

    void setThis();

    bool hasIdleThreads() { return m_idleThreadCount > 0; }

private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb)
        {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread
    {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f)
            , thread(thr)
        {}

        FiberAndThread(Fiber::ptr* f, int thr)
            : thread(thr)
        {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
            : cb(f)
            , thread(thr)
        {}

        FiberAndThread(std::function<void()>* f, int thr)
            : thread(thr)
        {
            cb.swap(*f);        
        }

        FiberAndThread()
            : thread(-1)
        {}

        void reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType m_mutex;
    /// 线程池
    std::vector<Thread::ptr> m_threads;
    /// 任务队列
    std::list<FiberAndThread> m_fibers;
    /// use_caller为true时有效，调度器所在线程的调用协程(但它是子协程)
    Fiber::ptr m_rootFiber;
    /// 协程调度器名称
    std::string m_name;

protected:
    /// 协程下的线程ID数组
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    /// 工作线程的数量
    std::atomic<size_t> m_activeThreadCount = {0};
    /// 空闲线程的数量
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = false;
    /// use_caller为true时，调度器所在线程的线程id
    int m_rootThread = 0;
};

class SchedulerSwitcher : public Noncopyable
{
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();

private:
    Scheduler* m_caller;
};

}

#endif
