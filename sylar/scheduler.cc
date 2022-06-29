#include "scheduler.h"
#include "fiber.h"
#include "macro.h"

namespace sylar
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
/// 当前线程的调度器，同一个调度器下的所有线程指向同一个调度器实例
static thread_local Scheduler* t_scheduler = nullptr;
/// 当前线程的调度协程，每个线程都独有一份，包括caller线程(caller线程的是当前线程的子协程)
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name)
{
    SYLAR_ASSERT(threads > 0);

    if(use_caller)
    {
        sylar::Fiber::GetThis();    // 这里面其实创建了当前线程的主协程
        --threads;

        SYLAR_ASSERT(GetThis() == nullptr);
        t_scheduler = this;
        
        // 创建调度协程（该线程的子协程）
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);
        
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    }
    else
    {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler()
{
    SYLAR_ASSERT(m_stopping);
    if(GetThis() == this)
    {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber()
{
    return t_scheduler_fiber;
}

void Scheduler::start()
{
    MutexType::Lock lk(m_mutex);
    if(!m_stopping)
    {
        return;
    }

    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    // 初始化线程池
    m_threads.resize(m_threadCount);
    for(size_t i=0; i<m_threadCount; ++i)
    {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
                    m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

void Scheduler::stop()
{
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT))
    {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping())
        {
            return;
        }
    }

    // 如果user_caller，则调用stop的线程也应该是它
    if(m_rootThread != -1)
    {
        SYLAR_ASSERT(GetThis() == this);
    }
    else
    {
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i=0; i<m_threadCount; ++i)
    {
        tickle();
    }

    if(m_rootFiber)
    {
        tickle();
    }

    if(m_rootFiber)
    {
        if(!stopping())
        {
            // 此时才切换到m_rootFiber这条协程执行
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lk(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs)
    {
        i->join();
    }
}

/**
 * @brief   切换线程（也可切换调度器）
 *
 * @param   thread 线程id
 */
void Scheduler::switchTo(int thread)
{
    SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this)
    {
        // 如果已经是当前调度器，而线程id未指定或已经是当前线程，则直接return掉
        if(thread == -1 || thread == sylar::GetThreadId())
        {
            return;
        }
    }

    // 再次加入任务队列里面，并让出当前协程的执行权
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os)
{
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";

    for(size_t i=0; i<m_threadIds.size(); ++i)
    {
        if(i)
        {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

void Scheduler::tickle()
{
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

void Scheduler::run()
{
    SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
    
    setThis();      // 设置当前调度器

    // 设置调度协程
    if(sylar::GetThreadId() != m_rootThread)
    {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true)
    {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lk(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end())
            {
                // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，
                // 然后跳过这个任务，继续下一个
                if(it->thread != -1 && it->thread != sylar::GetThreadId())
                {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);

                if(it->fiber && it->fiber->getState() == Fiber::EXEC)
                {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }

            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么标记一下需要通知其他线程进行调度
            tickle_me |= it!=m_fibers.end();
        }

        if(tickle_me)
        {
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT))
        {
            ft.fiber->swapIn();         // 切进去执行该协程
            --m_activeThreadCount;      // 等该协程出来的时候，活跃线程数便可以减1

            // 该协程出来后，如果还是READY状态，则再把它进入到任务队列中
            if(ft.fiber->getState() == Fiber::READY)
            {
                schedule(ft.fiber);
            }
            else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT)
            {
                ft.fiber->setState(Fiber::HOLD);
            }
            ft.reset();
        }
        else if(ft.cb)      // 任务包装的是函数对象
        {
            if(cb_fiber)
            {
                cb_fiber->reset(ft.cb);
            }
            else
            {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY)
            {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM)
            {
                cb_fiber->reset(nullptr);
            }
            else
            {
                cb_fiber->setState(Fiber::HOLD);
                cb_fiber.reset();
            }
        }
        else
        {
            if(is_active)
            {
                --m_activeThreadCount;
                continue;
            }

            if(idle_fiber->getState() == Fiber::TERM)
            {
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();       // 执行idle协程，其实是在忙等
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT)
            {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }
}

bool Scheduler::stopping()
{
    MutexType::Lock lk(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle()
{
    SYLAR_LOG_INFO(g_logger) << "idle";
    while(!stopping())
    {
        sylar::Fiber::YieldToHold();
    }
}

void Scheduler::setThis()
{
    t_scheduler = this;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
{
    m_caller = Scheduler::GetThis();
    if(target)
    {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher()
{
    if(m_caller)
    {
        m_caller->switchTo();
    }
}

}
