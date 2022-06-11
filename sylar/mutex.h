/**
 * @filename    mutex.h
 * @brief   封装信号量、互斥量、读写锁、自旋锁、原子锁以及利用RAII思想对它们的包装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <memory>
#include <atomic>
#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>

#include "noncopyable.h"

namespace sylar
{

template<class T>
struct ScopedLockImpl
{
    ScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl
{
    ReadScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl
{
    WriteScopedLockImpl(T& mutex)
        : m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

/**
 * @brief   信号量
 */
class Semaphore : public Noncopyable
{
public:

    /**
     * @brief   构造函数
     *
     * @param   count   信号量值的大小
     */
    Semaphore(uint32_t count = 0);

    ~Semaphore();


    /**
     * @brief   获取信号量
     */
    void wait();


    /**
     * @brief   释放信号量
     */
    void notify();

private:
    sem_t m_semaphore;
};

class Mutex : public Noncopyable
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;
};

class RWMutex : Noncopyable
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }

    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }

    void rwlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

class Spinlock : Noncopyable
{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock()
    {
        pthread_spin_init(&m_lock, 0);
    }

    ~Spinlock()
    {
        pthread_spin_destroy(&m_lock);
    }

    void lock()
    {
        pthread_spin_lock(&m_lock);
    }

    void unlock()
    {
        pthread_spin_unlock(&m_lock);
    }

private:
    pthread_spinlock_t m_lock;
};

class CASLock : Noncopyable
{
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock()
    {
        m_flag.clear();
    }

    ~CASLock()
    {}

    void lock()
    {
        while(std::atomic_flag_test_and_set_explicit(&m_flag, std::memory_order_acquire));
    }

    void unlock()
    {
        std::atomic_flag_clear_explicit(&m_flag, std::memory_order_release);
    }

private:
    volatile std::atomic_flag m_flag;
};

}


#endif
