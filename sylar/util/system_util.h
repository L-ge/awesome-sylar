/**
 * @filename    system_util.h
 * @brief   常用的系统工具函数
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_SYSTEM_UTIL_H__
#define __SYLAR_SYSTEM_UTIL_H__

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>

namespace sylar
{

pid_t GetThreadId();

uint32_t GetFiberId();

}

#endif
