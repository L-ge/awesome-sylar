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
#include <string>
#include <vector>

namespace sylar
{

pid_t GetThreadId();
uint32_t GetFiberId();

std::string demangle(const char* str);
void Backtrace(std::vector<std::string>& bt, int size, int skip);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");


/**
 * @brief  获取当前时间的毫秒 
 */
uint64_t GetCurrentMS();

/**
 * @brief   获取当前时间的微秒
 */
uint64_t GetCurrentUS();

}

#endif
