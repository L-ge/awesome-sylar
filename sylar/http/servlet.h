/**
 * @filename    servlet.h
 * @brief   Servlet 模块
 * @author  L-ge
 * @version 0.1
 * @modify  2022-07-16
 */
#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "sylar/mutex.h"
#include "sylar/util.h"

namespace sylar
{

namespace http
{

/**
 * @brief   Servlet封装
 */
class Servlet
{
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        : m_name(name)
    {}
    virtual ~Servlet() {}

    /**
     * @brief   处理请求
     *
     * @param   request http请求
     * @param   rsponse http响应
     * @param   session http连接
     *
     * @return  是否处理成功
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session) = 0;

    const std::string& getName() const { return m_name; }

protected:
    /// 名称(一般用于打印日志区分不同的Servlet)
    std::string m_name;
};

/**
 * @brief   函数式Servlet
 */
class FunctionServlet : public Servlet
{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    /// 函数回调类型的定义
    typedef std::function< int32_t (sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session)> callback;

    /**
     * @brief   构造函数
     *
     * @param   cb  回调函数
     */
    FunctionServlet(callback cb);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session) override;

private:
    /// 回调函数
    callback m_cb;
};

class IServletCreator
{
public:
    typedef std::shared_ptr<IServletCreator> ptr;
    virtual ~IServletCreator() {}
    virtual Servlet::ptr get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator 
{
public:
    typedef std::shared_ptr<HoldServletCreator> ptr;
    
    HoldServletCreator(Servlet::ptr slt)
        : m_servlet(slt) 
    {}

    Servlet::ptr get() const override { return m_servlet; }
    std::string getName() const override { return m_servlet->getName(); }

private:
    Servlet::ptr m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator
{
public:
    typedef std::shared_ptr<ServletCreator> ptr;

    ServletCreator() {}

    Servlet::ptr get() const override { return Servlet::ptr(new T); }
    std::string getName() const override { return TypeToName<T>(); }
};

/**
 * @brief   Servlet分发器
 */
class ServletDispatch : public Servlet
{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief   添加模糊匹配Servlet
     *
     * @param   uri 模糊匹配 /sylar_*
     * @param   slt Servlet
     */
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    
    /**
     * @brief   添加模糊匹配Servlet
     *
     * @param   uri 模糊匹配 /sylar_*
     * @param   cb  FunctionServlet回调函数
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);
    void addServletCreatot(const std::string& uri, IServletCreator::ptr creator);
    void addGlobServletCreatot(const std::string& uri, IServletCreator::ptr creator);

    template<class T>
    void addServletCreator(const std::string& uri)
    {
        addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    template<class T>
    void addGlobServletCreator(const std::string& uri)
    {
        addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief   通过uri获取Servlet
     *
     * @param   uri uri
     *
     * @return  优先精准匹配，其次模糊匹配，最后返回默认
     */
    Servlet::ptr getMatchedServlet(const std::string& uri);

    void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos);

private:
    /// 读写互斥量
    RWMutexType m_mutex;
    /// 精准匹配Servlet map
    /// uri(/sylar/xxx) -> servlet
    std::unordered_map<std::string, IServletCreator::ptr> m_datas;
    /// 模糊匹配Servlet 数组
    /// uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;
    /// 默认servlet，所有路径都没匹配到时使用 
    Servlet::ptr m_default;
};

/**
 * @brief   NotFoundServlet(默认返回404)
 */
class NotFoundServlet : public Servlet
{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet(const std::string& name);

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         , sylar::http::HttpResponse::ptr response
                         , sylar::http::HttpSession::ptr session) override;

private:
    std::string m_name;
    std::string m_content;
};

}

}

#endif
