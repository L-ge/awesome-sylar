/**
 * @filename    fiber.h
 * @brief   协程模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-26
 */
#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace sylar
{

/**
 * @brief   协程类
 */
class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
    typedef std::shared_ptr<Fiber> ptr;

    // 协程运行状态
    enum State
    {
        INIT,       // 初始化状态
        HOLD,       // 暂停状态
        EXEC,       // 执行中状态
        TERM,       // 结束状态
        READY,      // 可执行(就绪)状态
        EXCEPT,     // 异常状态
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    void reset(std::function<void()> cb);
    void swapIn();
    void swapOut();
    void call();
    void back();
    
    uint64_t getId() const { return m_id; }
    void setState(State s) { m_state = s; }
    State getState() const { return m_state; }

public:
    static void SetThis(Fiber* f);
    static Fiber::ptr GetThis();
    
    static void YieldToReady();
    static void YieldToHold();
    
    static uint64_t TotalFibers();
    static uint64_t GetFiberId();

    static void MainFunc();
    static void CallerMainFunc();

private:
    /// 协程id
    uint64_t m_id = 0;
    /// 协程运行栈大小
    uint32_t m_stacksize = 0;
    /// 协程状态
    State m_state = INIT;
    /// 协程上下文
    ucontext_t m_ctx;
    /// 协程运行栈指针
    void* m_stack = nullptr;
    /// 协程运行函数
    std::function<void()> m_cb;
};

}

#endif 
