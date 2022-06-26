#include "fiber.h"
#include "config.h"
#include "scheduler.h"
#include "macro.h"
#include <atomic>

namespace sylar
{

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};         // 用于生成协程ID
static std::atomic<uint64_t> s_fiber_count{0};      // 用于统计协程数

static thread_local Fiber* t_fiber = nullptr;           // 当前线程正在运行的协程
static thread_local Fiber::ptr t_threadFiber = nullptr; // 当前线程的主协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 128*1024, "fiber stack size");

class MallocStackAllocator
{
public:
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size)
    {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}
    
/**
 * @brief  返回当前线程正在执行的协程。
 *         如果当前线程尚未创建协程，则创建该线程的第一个协程，且该协程为当前线程的主协程，其他协程都通过这个协程来调度。
 */
Fiber::ptr Fiber::GetThis()
{
    if(t_fiber)
    {
        return t_fiber->shared_from_this();
    }
    
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}
    
/**
 * @brief   将当前协程切换到后台，并设置为 READY 状态
 */
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

/**
 * @brief   将当前协程切换到后台，并设置为 HOLD 状态
 */
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->m_state == EXEC);
    cur->swapOut();
}
    
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}
    
uint64_t Fiber::GetFiberId()
{
    if(t_fiber)
    {
        t_fiber->getId();
    }
    return 0;
}

/**
 * @brief   协程执行函数，执行完成返回到线程的主协程
 */
void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis(); 
    SYLAR_ASSERT(cur);
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& ex)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    catch(...)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();        // 引用计数减1
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
    
/**
 * @brief   协程执行函数，执行完成返回到线程调度协程
 *          协程的调度是运行在 caller 线程的子协程中，
 *          因此这里返回到的是 caller 线程的主协程中。
 */
void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    try
    {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& ex)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }
    catch(...)
    {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
            << " fiber_id=" << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

/**
 * @brief   只用于创建线程的第一个协程，也就是线程主函数对应的协程
 */
Fiber::Fiber()
{
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");        
    }

    ++s_fiber_count;

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id)
    , m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");        
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller)
    {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }
    else
    {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}
    
Fiber::~Fiber()
{
    --s_fiber_count;
    if(m_stack)
    {
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else
    {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this)
        {
            SetThis(nullptr);
        }
    }

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << " total=" << s_fiber_count;
}

/**
 * @brief  重置协程，即重复利用已结束的协程，复用其栈空间，创建新协程 
 */
void Fiber::reset(std::function<void()> cb)
{
    m_cb = cb;
    if(getcontext(&m_ctx))
    {
        SYLAR_ASSERT2(false, "getcontext");        
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

/**
 * @brief   将当前协程切换到运行状态
 */
void Fiber::swapIn()
{
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx))
    {
        SYLAR_ASSERT2(false, "swapcontext");        
    }
}
    
/**
 * @brief   将当前协程切换到后台
 */
void Fiber::swapOut()
{
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx))
    {
        SYLAR_ASSERT2(false, "swapcontext");        
    }
}
    
/**
 * @brief   将当前线程切换到执行状态
 *          当前线程的主协程切换到当前协程
 */
void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx))
    {
        SYLAR_ASSERT2(false, "swapcontext");        
    }
}
    
/**
 * @brief   将当前线程切换到后台
 *          当前协程切换为当前线程的主协程
 */
void Fiber::back()
{
    SetThis(this);
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx))
    {
        SYLAR_ASSERT2(false, "swapcontext");        
    }
}

}
