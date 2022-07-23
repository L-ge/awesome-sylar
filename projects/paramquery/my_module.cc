#include "my_module.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/application.h"
#include "sylar/env.h"

#include "paramquery_servlet.h"

namespace paramquery 
{

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

MyModule::MyModule()
    : sylar::Module("paramquery", "1.0", "") 
{
}

bool MyModule::onLoad() 
{
    SYLAR_LOG_INFO(g_logger) << "onLoad";
    return true;
}

bool MyModule::onUnload() 
{
    SYLAR_LOG_INFO(g_logger) << "onUnload";
    return true;
}


bool MyModule::onServerReady() 
{
    SYLAR_LOG_INFO(g_logger) << "onServerReady";
    
    std::vector<sylar::TcpServer::ptr> svrs;
    if(!sylar::Application::GetInstance()->getServer("http", svrs)) 
    {
        SYLAR_LOG_INFO(g_logger) << "no httpserver alive";
        return false;
    }

    for(auto& i : svrs)
    {
        sylar::http::HttpServer::ptr http_server = std::dynamic_pointer_cast<sylar::http::HttpServer>(i);
        if(!i) 
        {
            continue;
        }
        
        auto slt_dispatch = http_server->getServletDispatch();

        sylar::http::ParamQueryServlet::ptr slt(new sylar::http::ParamQueryServlet(
                    sylar::EnvMgr::GetInstance()->getCwd()
        ));
        slt_dispatch->addGlobServlet("/paramquery", slt);
        SYLAR_LOG_INFO(g_logger) << "addServlet";
    }

    return true;
}

bool MyModule::onServerUp()
{
    SYLAR_LOG_INFO(g_logger) << "onServerUp";
    return true;
}

}

extern "C" {

sylar::Module* CreateModule()
{
    sylar::Module* module = new paramquery::MyModule;
    SYLAR_LOG_INFO(paramquery::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(sylar::Module* module) {
    SYLAR_LOG_INFO(paramquery::g_logger) << "DestoryModule " << module;
    delete module;
}

}

