/**
 * @filename    noncopyable.h
 * @brief   不可拷贝对象封装
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace sylar
{

class Noncopyable
{
public:
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
