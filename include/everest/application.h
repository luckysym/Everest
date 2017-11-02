#ifndef INCLUDED_EVEREST_APPLICATION_H
#define INCLUDED_EVEREST_APPLICATION_H

#pragma once 

#include <string>
#include <memory>
#include <set>

#include <everest/properties.h>

namespace everest
{
    class Application;
    class ApplicationContext;   
    
    class ApplicationContext
    {
    public:
        static const int RSIG_STOP = 1;    // 收到停止信号
        
    private:
        int        m_result;      // 程序运行结果值
        int        m_rsignals;    // 收到的信号
        Properties m_props;
        
    public:
        ApplicationContext();
        ~ApplicationContext();
        
        Properties & properties() { return m_props; }
        const Properties & properties() const { return m_props; }
        
        int result() const { return m_result; }
        void result(int ret) { m_result = ret; }
        
        bool signal_stop() const  { return m_rsignals & RSIG_STOP; }   // 是否收到停止信号
    }; // end of ApplicationContext
    
    class Application
    {
    public:
        typedef int (*PFN_MAIN)(ApplicationContext &);
    
    private:
        ApplicationContext * m_ctx;
        
    public:
        Application(int argc, char **argv);
        ~Application();
        
        void run(PFN_MAIN proc);
        int  result() const { return m_ctx->result(); }
    }; // end of class Application
    

    
    
} // end of namespace everest 

#endif // INCLUDED_EVEREST_APPLICATION_H