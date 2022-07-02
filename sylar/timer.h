/**
 * @filename    timer.h
 * @brief   定时器模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-02
 */
#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>

#include "thread.h"

namespace sylar
{

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer>
{
friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();
    bool refresh();
    bool reset(uint64_t ms, bool from_now);

private:
    /**
     * @brief   通过相对时间点构造定时器
     *
     * @param   ms  定时器的执行时间间隔（相对时间点）
     * @param   cb  回调函数
     * @param   recurring   是否循环执行
     * @param   manager 定时器管理器
     */
    Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);

    /**
     * @brief   通过绝对时间点构造定时器
     *
     * @param   next    执行的时间戳（绝对时间点）
     */
    Timer(uint64_t next);

private:
    /// 是否循环定时器
    bool m_recurring = false;
    /// 执行周期
    uint64_t m_ms = 0;
    /// 精确的执行时间
    uint64_t m_next = 0;
    /// 回调函数
    std::function<void()> m_cb;
    /// 定时器管理器
    TimerManager* m_manager = nullptr;

private:
    /**
     * @brief   定时器比较仿函数
     */
    struct Comparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager
{
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    /**
     * @brief  添加定时器 
     *
     * @param   ms  定时器执行的时间间隔
     * @param   cb  定时器回调函数
     * @param   recurring   是否是循环执行的定时器
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief   添加条件定时器
     *
     * @param   ms  定时器执行的时间间隔
     * @param   cb  定时器回调函数
     * @param   weak_cond   条件
     * @param   recurring   是否是循环执行的定时器
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, 
            std::weak_ptr<void> weak_cond, bool recurring = false);

    /**
     * @brief   获取最近一个定时器执行的时间间隔
     */
    uint64_t getNextTimer();

    /**
     * @brief   获取需要执行的定时器的回调函数的集合
     */
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    /**
     * @brief   是否有定时器
     */
    bool hasTimer();

protected:
    /**
     * @brief  当有新的定时器插入到定时器的最前面时，执行该函数 
     */
    virtual void onTimerInsertedAtFront() = 0;
    
    /**
     * @brief   将定时器添加到定时器管理器中
     */
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

private:
    /**
     * @brief   检测服务器时间是否被调后了
     */
    bool detectClockRollover(uint64_t now_ms);

private:
    RWMutexType m_mutex;
    /// 定时器集合（自定义比较函数）
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    /// 是否触发 onTimerInsertedAtFront
    bool m_tickled = false;
    /// 上次执行时间
    uint64_t m_previousTime = 0;
};

}

#endif
