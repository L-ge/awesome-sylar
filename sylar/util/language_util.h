#ifndef __SYLAR_LANGUAGE_UTIL_H__
#define __SYLAR_LANGUAGE_UTIL_H__

#include <cxxabi.h>

namespace sylar
{

template<class T>
const char* TypeToName()
{
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}

}

#endif
