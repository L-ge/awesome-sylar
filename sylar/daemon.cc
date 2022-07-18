#include "daemon.h"
#include "sylar/log.h"
#include "sylar/config.h"
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace sylar 
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
static sylar::ConfigVar<uint32_t>::ptr g_daemon_restart_interval
    = sylar::Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const 
{
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << sylar::Time2Str(parent_start_time)
       << " main_start_time=" << sylar::Time2Str(main_start_time)
       << " restart_count=" << restart_count << "]";
    return ss.str();
}

static int real_start(int argc, char** argv,
                     std::function<int(int argc, char** argv)> main_cb)
{
    ProcessInfoMgr::GetInstance()->main_id = getpid();
    ProcessInfoMgr::GetInstance()->main_start_time = time(0);
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv,
                     std::function<int(int argc, char** argv)> main_cb) 
{
    // #include <unistd.h>
    // int daemon(int nochdir, int noclose);
    // 当 nochdir 为0时，daemon 将更改进程的工作目录为根目录root("/")。
    // 当noclose为0时，daemon将进程的STDIN, STDOUT, STDERR都重定向到/dev/null，也就是不输出任何信息，否则照样输出。一般情况下，这个参数都是设为0的。
    // deamon()调用了fork()，如果fork成功，父进程在daemon函数运行完毕后自杀，子进程由init进程领养。此时的子进程，其实就是守护进程了。
    daemon(1, 0);   // 创建守护进程
    ProcessInfoMgr::GetInstance()->parent_id = getpid();
    ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
    while(true) 
    {
        pid_t pid = fork();
        if(pid == 0) 
        {
            //子进程返回
            ProcessInfoMgr::GetInstance()->main_id = getpid();
            ProcessInfoMgr::GetInstance()->main_start_time  = time(0);
            SYLAR_LOG_INFO(g_logger) << "process start pid=" << getpid();
            return real_start(argc, argv, main_cb);
        } 
        else if(pid < 0) 
        {
            SYLAR_LOG_ERROR(g_logger) << "fork fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno);
            return -1;
        }
        else
        {
            //父进程返回
            int status = 0;
            waitpid(pid, &status, 0);
            if(status)
            {
                if(status == 9)
                {
                    SYLAR_LOG_INFO(g_logger) << "killed";
                    break;
                }
                else 
                {
                    SYLAR_LOG_ERROR(g_logger) << "child crash pid=" << pid
                        << " status=" << status;
                }
            } 
            else 
            {
                SYLAR_LOG_INFO(g_logger) << "child finished pid=" << pid;
                break;
            }
            ProcessInfoMgr::GetInstance()->restart_count += 1;
            // 防止子进程死掉之后资源还没有被全部回收，
            // 此时又马上fork出子进程可能会有问题。
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv
                 , std::function<int(int argc, char** argv)> main_cb
                 , bool is_daemon) 
{
    if(!is_daemon) 
    {
        ProcessInfoMgr::GetInstance()->parent_id = getpid();
        ProcessInfoMgr::GetInstance()->parent_start_time = time(0);
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}

}

