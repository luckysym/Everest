#ifndef INCLUDE_EVEREST_RPC_RPC_SERVER_H
#define INCLUDE_EVEREST_RPC_RPC_SERVER_H

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <stdexcept>
#include <boost/asio.hpp>

    
namespace everest
{

namespace log 
{
    const char * g_arrLogLevels[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    class Logger
    {
    public:
        bool error_enabled() { return true; }
    
        bool write(int level, const char *msg) {
            std::cout<<'['<<g_arrLogLevels[level]<<"] "<<msg<<std::endl;
            return true;
        }
        
    }; // end of class Logger
    
    class Log_Stream
    {
    private:
        Logger & m_logger;
        int      m_level;
        std::stringstream m_stream;
        
    public:
        Log_Stream(Logger & log, int level) 
            : m_logger(log), m_level(level)
        {}
          
        ~Log_Stream() {}
        
        Log_Stream& operator<<(const char *str) 
        {
            m_stream<<str;
        }
        
        void flush() {
            m_logger.write(m_level, m_stream.str().c_str());
        }
        
    };  // end of class LogStream 
    
    typedef std::shared_ptr<Logger> LoggerPtr; 
    
    static const int Trace = 0;
    static const int Debug = 1;
    static const int Info  = 2;
    static const int Warn  = 3;
    static const int Error = 4;
    static const int Fatal = 5;
    
    LoggerPtr GetLogger() { return LoggerPtr(new Logger()); }
    
} // end of namespace log 
    
namespace rpc 
{
    struct Tcp_v4
    {
        typedef boost::asio::ip::tcp::acceptor Acceptor;
        typedef boost::asio::ip::tcp::socket   Socket;
        typedef boost::asio::ip::tcp::endpoint Endpoint;
        typedef boost::asio::ip::address_v4    Address;
        
        static boost::asio::ip::tcp  get() { return boost::asio::ip::tcp::v4(); }
    };

    template<class Protocol = Tcp_v4>
    class RPC_SocketService final
    {
    private:
        RPC_SocketService(const RPC_SocketService&) = delete;
        RPC_SocketService& operator=(const RPC_SocketService&) = delete;
        
    public:
        typedef Protocol                                      ProtocolType;
        typedef typename Protocol::Acceptor                   Acceptor;
        typedef typename Protocol::Socket                     Socket;
        typedef typename Protocol::Address                    Address;
        typedef typename Protocol::Endpoint                   Endpoint;
        typedef std::shared_ptr<typename Protocol::Acceptor>  AcceptorPtr;
        typedef std::shared_ptr<typename Protocol::Socket>    SocketPtr;
        
        class AsyncConnectHandler
        {
        private:
            RPC_SocketService& m_rService;
            SocketPtr          m_ptrSocket;
            
        public:
            AsyncConnectHandler(RPC_SocketService& service, SocketPtr &sockPtr) 
                : m_rService(service), m_ptrSocket(sockPtr) 
            {}
            
            ~AsyncConnectHandler() {}
            
            void operator() (const boost::system::error_code& ec) {
                if ( m_rService.m_connHandler ) {
                    m_rService.m_connHandler(m_rService, m_ptrSocket, !ec);
                } else {
                    m_rService.m_ptrLog->write(log::Error, "async connect handler not callable");
                    throw std::runtime_error("async connect handler not callable");
                }
                return ;
            }
        }; // end of class AsyncConnectHandler

    private:
        boost::asio::io_service         m_ioservice;
        std::unordered_set<AcceptorPtr> m_setAcceptor;
        std::unordered_set<SocketPtr>   m_setSocket; 
        log::LoggerPtr                  m_ptrLog;
        std::function<void(RPC_SocketService &, SocketPtr &, bool)> m_connHandler;
        
    public:
        RPC_SocketService();
        ~RPC_SocketService();

        AcceptorPtr open_listener(const char * endpoint);
        SocketPtr   open_channel(const char * endpoint);
        
        template<class Handler>
        void set_conn_handler(Handler h) { m_connHandler = h; }
        
        int run();
        
    }; // end of class RPC_Server
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest
{
namespace rpc 
{
    template<class Protocol>
    RPC_SocketService<Protocol>::RPC_SocketService() 
        : m_ptrLog(log::GetLogger())
    {}
    
    template<class Protocol>
    RPC_SocketService<Protocol>::~RPC_SocketService() {}
    
    template<class Protocol>
    int RPC_SocketService<Protocol>::run() {
        boost::system::error_code ec;
        int ret = (int)m_ioservice.run(ec);
        if ( ec )  {
            if ( m_ptrLog->error_enabled() ) {
                m_ptrLog->write(log::Error, "io service run fail");
                return -1;
            }
        }
        return ret;
    }
    
    template<class Protocol>
    typename RPC_SocketService<Protocol>::AcceptorPtr
    RPC_SocketService<Protocol>::open_listener(const char *endpoint) 
    {
        const char * pos = strchr(endpoint, ':');
        if ( pos == nullptr ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "bad endpoint string, addr:port, %s", endpoint);
                m_ptrLog->write(log::Error, buf);
            }
            return AcceptorPtr(nullptr);
        }
        char * addr = strndup(endpoint, pos - endpoint);
        assert(addr);
        
        Address  address;
        boost::system::error_code ec;
        address.from_string(addr, ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "bad address string, %s", addr);
                m_ptrLog->write(log::Error, buf);
            }
            free(addr);
            return AcceptorPtr(nullptr);
        }
        free(addr);
        
        AcceptorPtr acceptorPtr(new Acceptor(m_ioservice));
        acceptorPtr->open(Protocol::get(), ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                m_ptrLog->write(log::Error, "failed to open acceptor");
            }
            return AcceptorPtr(nullptr);
        }
        
        int port = atoi(pos + 1);
        Endpoint ep(address, port);
        acceptorPtr->bind(ep, ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "failed to bind, %s, %d", ep.address().to_string().c_str(), port);
                m_ptrLog->write(log::Error, buf);
            }
            return AcceptorPtr(nullptr);
        }
        acceptorPtr->listen(boost::asio::socket_base::max_connections, ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "listen failed, %s", endpoint);
                m_ptrLog->write(log::Error, buf);
            }
            return AcceptorPtr(nullptr);;
        }
        m_setAcceptor.insert(acceptorPtr);
        
        return acceptorPtr;
    } // end of RPC_SocketService<Protocol>::open_listener(const char *endpoint) 
    
    template<class Protocol>
    typename RPC_SocketService<Protocol>::SocketPtr
    RPC_SocketService<Protocol>::open_channel(const char *endpoint)
    {
        const char * pos = strchr(endpoint, ':');
        if ( pos == nullptr ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "bad endpoint string, addr:port, %s", endpoint);
                m_ptrLog->write(log::Error, buf);
            }
            return SocketPtr(nullptr);
        }
        char * addr = strndup(endpoint, pos - endpoint);
        assert(addr);
        
        Address  address;
        boost::system::error_code ec;
        address.from_string(addr, ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                char buf[128];
                snprintf(buf, 128, "bad remote address string, %s", addr);
                m_ptrLog->write(log::Error, buf);
            }
            free(addr);
            return SocketPtr(nullptr);
        }
        free(addr);
        
        SocketPtr socketPtr(new Socket(m_ioservice));
        socketPtr->open(Protocol::get(), ec);
        if ( ec ) {
            if ( m_ptrLog->error_enabled() ) {
                m_ptrLog->write(log::Error, "failed to open socket");
            }
            return SocketPtr(nullptr);
        }
        int port = atoi(pos + 1);
        Endpoint ep(address, port);
        socketPtr->async_connect(ep, AsyncConnectHandler(*this, socketPtr));
        m_setSocket.insert(socketPtr);
        
        return socketPtr;
    } // end of RPC_SocketService<Protocol>::open_channel(const char *endpoint)
    
} // end of namespace rpc 
} // end of namespace everest 

    

#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
