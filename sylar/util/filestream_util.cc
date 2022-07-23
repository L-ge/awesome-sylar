#include "filestream_util.h"
#include <dirent.h>
#include <sys/types.h>
#include <signal.h>

namespace sylar
{

static int __lstat(const char* file, struct stat* st = nullptr)
{
    // lstat函数是获取参数file所指的文件状态，
    // 当文件为符合链接时，该函数会返回该link本身的状态。
    struct stat lst;
    int ret = lstat(file, &lst);
    if(st)
    {
        *st = lst;
    }

    return ret;
}

static int __mkdir(const char* dirname)
{
    if(access(dirname, F_OK) == 0)
    {
        return 0;
    }

    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FSUtil::Mkdir(const std::string& dirname)
{
    if(__lstat(dirname.c_str()) == 0)
    {
        return true;
    }

    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path+1, '/');
    do
    {
        for(; ptr; *ptr = '/', ptr = strchr(ptr+1, '/'))
        {
            *ptr = '\0';
            if(__mkdir(path) != 0)
            {
                break;
            }
        }

        if(ptr != nullptr)
        {
            break;
        }
        else if(__mkdir(path) != 0)
        {
            break;
        }

        free(path);
        return true;
    }while(0);

    free(path);
    return false;
}

std::string FSUtil::Dirname(const std::string& filename)
{
    if(filename.empty())
    {
        return ".";
    }
    auto pos = filename.rfind('/');
    if(pos == 0)
    {
        return "/";
    }
    else if(pos == std::string::npos)
    {
        return ".";
    }
    
    return filename.substr(0, pos);
}

bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode)
{
    ofs.open(filename.c_str(), mode);
    if(!ofs.is_open())
    {
        std::string dir = Dirname(filename);
        Mkdir(dir);
        ofs.open(filename.c_str(), mode);
    }

    return ofs.is_open();
}

void FSUtil::ListAllFile(std::vector<std::string>& files, const std::string& path, const std::string& subfix)
{
    if(access(path.c_str(), 0) != 0)
    {
        return;
    }
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr)
    {
        return;
    }
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)) != nullptr)
    {
        if(dp->d_type == DT_DIR)
        {
            if(!strcmp(dp->d_name, ".")
                    || !strcmp(dp->d_name, ".."))
            {
                continue;
            }
            ListAllFile(files, path+"/"+dp->d_name, subfix);
        }
        else if(dp->d_type == DT_REG)
        {
            std::string filename(dp->d_name);
            if(subfix.empty())
            {
                files.push_back(path+"/"+filename);
            }
            else
            {
                if(filename.size() < subfix.size())
                {
                    continue;
                }
                if(filename.substr(filename.length()-subfix.size()) == subfix)
                {
                    files.push_back(path+"/"+filename);
                }
            }
        }
    }
    closedir(dir);
}

bool FSUtil::Unlink(const std::string& filename, bool exist)
{
    if(!exist && __lstat(filename.c_str()))
    {
        return false;
    }
    // unlink函数：从文件系统中删除一个名称。
    // 如果名称是文件的最后一个连接，并且没有其他进程将文件打开，名称对应的文件会实际被删除。
    return ::unlink(filename.c_str()) == 0;
}

bool FSUtil::IsRunningPidfile(const std::string& pidfile) 
{
    if(__lstat(pidfile.c_str()) != 0) 
    {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) 
    {
        return false;
    }
    if(line.empty()) 
    {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) 
    {
        return false;
    }
    if(kill(pid, 0) != 0)
    {
        return false;
    }
    return true;
}

bool FSUtil::Rm(const std::string& path)
{
    struct stat st;
    if(lstat(path.c_str(), &st)) 
    {
        return true;
    }
    
    if(!(st.st_mode & S_IFDIR))
    {
        return Unlink(path);
    }

    DIR* dir = opendir(path.c_str());
    if(!dir) 
    {
        return false;
    }
    
    bool ret = true;
    struct dirent* dp = nullptr;
    while((dp = readdir(dir)))
    {
        if(!strcmp(dp->d_name, ".")
                || !strcmp(dp->d_name, "..")) 
        {
            continue;
        }
        std::string dirname = path + "/" + dp->d_name;
        ret = Rm(dirname);
    }
    closedir(dir);
    if(::rmdir(path.c_str()))
    {
        ret = false;
    }
    return ret;
}

bool FSUtil::Mv(const std::string& from, const std::string& to)
{
    if(!Rm(to))
    {
        return false;
    }
    return rename(from.c_str(), to.c_str()) == 0;
}

}
