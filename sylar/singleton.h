/**
 * @filename    singleton.h
 * @brief   单例模式封装类
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__

#include <memory>

namespace sylar
{

template<class T, class X, int N>
T& GetInstanceX()
{
    static T v;
    return v;
}

template<class T, class X, int N>
std::shared_ptr<T> GetInstancePtr()
{
    static std::shared_ptr<T> v(new T);
    return v;
}

/**
 * @brief   单例模式封装类
 *
 * @tparam t    类型
 * @tparam x    为了创造多个实例对应的tag
 * @tparam n    同一个tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class Singleton
{
public:
    static T* GetInstance()
    {
        static T v;
        return &v;
    }
};

/**
 * @brief   单例模式智能指针封装类
 *
 * @tparam t    类型
 * @tparam x    为了创造多个实例对应的tag
 * @tparam n    同一个tag创造多个实例索引
 */
template<class T, class X = void, int N = 0>
class SingletonPtr
{
public:
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}

#endif
