/**
 * @filename    filestream_util.h
 * @brief   常用的文件流工具函数
 * @author  L-ge
 * @version 0.1
 * @modify  2022-06-11
 */
#ifndef __SYLAR_FILESTREAM_UTIL_H__
#define __SYLAR_FILESTREAM_UTIL_H__

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <cxxabi.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>

namespace sylar
{

class FSUtil
{
public:
    static bool Mkdir(const std::string& dirname);
    static std::string Dirname(const std::string& filename);
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode);
    static void ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix);
    static bool Unlink(const std::string& filename, bool exist = false);
    static bool IsRunningPidfile(const std::string& pidfile);
    static bool Rm(const std::string& path);
    static bool Mv(const std::string& from, const std::string& to);
};

}

#endif
