#include "timer.h"
#include "util.h"

namespace sylar
{

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    : m_recurring(recurring)
    , m_ms(ms)
    , m_cb(cb)
    , m_manager(manager)
{
    m_next = sylar::GetCurrentMS() + m_ms;      // 计算绝对时间点
}

Timer::Timer(uint64_t next)
    : m_next(next)          // 参数直接就是绝对时间点
{

}

bool Timer::cancel()
{
    TimerManager::RWMutexType::WriteLock lk(m_manager->m_mutex);
    if(m_cb)    // m_cb 为空，则表示该定时器已经超时了，执行过了
    {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh()
{
    TimerManager::RWMutexType::WriteLock lk(m_manager->m_mutex);
    if(!m_cb)  // m_cb 为空，则表示该定时器已经超时了，执行过了
    {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end())
    {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now)
{
    if(ms == m_ms && !from_now)
    {
        return true;
    }

    TimerManager::RWMutexType::WriteLock lk(m_manager->m_mutex);
    if(!m_cb)   // m_cb 为空，则表示该定时器已经超时了，执行过了
    {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end())
    {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now)
    {
        start = sylar::GetCurrentMS();
    }
    else
    {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lk);
    return true;
}

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
    if(!lhs && !rhs)                // 都是空，认为不小于
    {
        return false;
    }
    if(!lhs)                        // 自己是空，认为小于
    {
        return true;
    }
    if(!rhs)                        // 对方是空，认为不小于
    {
        return false;
    }
    if(lhs->m_next < rhs->m_next)   // 自己的绝对时间小，认为小于
    {
        return true;
    }
    if(rhs->m_next < lhs->m_next)   // 对方的绝对时间小，认为不小于
    {
        return false;
    }
    return lhs.get() < rhs.get();   // 其他情况，比较裸指针的大小
}

TimerManager::TimerManager()
{
    m_previousTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager()
{

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lk(m_mutex);
    addTimer(timer, lk);
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
{
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp)
    {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, 
                                           std::weak_ptr<void> weak_cond, bool recurring)
{
    // 通过std::bind，包装出一个新的回调函数
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer()
{
    RWMutexType::ReadLock lk(m_mutex);
    m_tickled = false;
    if(m_timers.empty())
    {
        return ~0ull;
    }
    
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();
    if(now_ms >= next->m_next)  // 已经超时了，要马上执行
    {
        return 0;
    }
    else
    {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs)
{
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lk(m_mutex);
        if(m_timers.empty())
        {
            return;
        }
    }

    RWMutexType::WriteLock lk(m_mutex);
    if(m_timers.empty())
    {
        return;
    }

    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) // 没有时间倒退且第一个定时器还没超时
    {
        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    // it 为第一个不小于 nowtimer 的定时器的迭代器
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms)  // 处理多个定时器时间相等的情况
    {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer : expired)
    {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring)
        {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        }
        else
        {
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::hasTimer()
{
    RWMutexType::ReadLock lk(m_mutex);
    return !m_timers.empty();
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock)
{
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front)
    {
        m_tickled = true;
    }
    lock.unlock();

    if(at_front)
    {
        onTimerInsertedAtFront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    if(now_ms < m_previousTime
            && now_ms < (m_previousTime - 60*60*1000)) // 小一个小时
    {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}

}
