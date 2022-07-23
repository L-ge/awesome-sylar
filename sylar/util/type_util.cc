#include "type_util.h"

namespace sylar
{

int8_t TypeUtil::ToChar(const std::string& str) 
{
    if(str.empty())
    {
        return 0;
    }
    return *str.begin();
}

int64_t TypeUtil::Atoi(const std::string& str)
{
    if(str.empty()) 
    {
        return 0;
    }
    return strtoull(str.c_str(), nullptr, 10);
}

double TypeUtil::Atof(const std::string& str) 
{
    if(str.empty())
    {
        return 0;
    }
    return atof(str.c_str());
}

int8_t TypeUtil::ToChar(const char* str)
{
    if(str == nullptr)
    {
        return 0;
    }
    return str[0];
}

int64_t TypeUtil::Atoi(const char* str) 
{
    if(str == nullptr)
    {
        return 0;
    }
    return strtoull(str, nullptr, 10);
}

double TypeUtil::Atof(const char* str)
{
    if(str == nullptr)
    {
        return 0;
    }
    return atof(str);
}

}
