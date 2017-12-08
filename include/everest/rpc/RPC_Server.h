#ifndef INCLUDE_EVEREST_RPC_RPC_SERVER_H
#define INCLUDE_EVEREST_RPC_RPC_SERVER_H

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>  
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <memory>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <stdexcept>
#include <queue>


namespace everest
{
namespace net 
{
    class SocketAddress final
    {
    private:
        char      m_addrbuf[128];
        socklen_t m_length;
    
    public:
        SocketAddress() : m_length(128) { }
        
        SocketAddress(int family) : m_length(128) {
            this->sock_addr().sa_family = family;
        }
        
        SocketAddress(const SocketAddress& ) = default;
        
        ~SocketAddress() {}
        
        SocketAddress& operator=(const SocketAddress&) = default;
        
        int family() const { 
            const sockaddr * p = (const sockaddr *)m_addrbuf;
            return  p->sa_family;
        }
        
        void family(int af) { 
            sockaddr * p = (sockaddr *)m_addrbuf;
            p->sa_family = af;
        }
        
        socklen_t length() const { return m_length; }
        
        void length(socklen_t len) { m_length = len; }
        
        operator const sockaddr & () const {
            return *((const sockaddr *)m_addrbuf);
        }
        
        operator sockaddr & () {
            return *((sockaddr *)m_addrbuf);
        }
        
        sockaddr & sock_addr() { 
            return *((sockaddr *)m_addrbuf);
        }
        
        const sockaddr & sock_addr() const { 
            return *((const sockaddr *)m_addrbuf);
        }
        
    }; // end of class SocketAddress 
    
    class InetAdderssAdapter final
    {
    private:
        SocketAddress &m_rAddr;
        
    public:
        InetAdderssAdapter(SocketAddress& addr) 
            : m_rAddr(addr)
        {}
        
        ~InetAdderssAdapter() {}
        
        SocketAddress& address() { return m_rAddr; }
        const SocketAddress& address() const  { return m_rAddr; }
        
        bool from_string(const char * endpoint);
    }; // end of class InetAdderssAdapter
    
    inline bool InetAdderssAdapter::from_string(const char * endpoint)
    {
        sockaddr * paddr = &m_rAddr.sock_addr();
        sockaddr_in * pinaddr = (sockaddr_in*)paddr;
        
        char * addr = ::strdup(endpoint);
        char * p = strchr(addr, ':');
        if ( p == nullptr ) {
            free(addr);
            printf("[ERROR] InetAdderssAdapter::from_string, bad endpoint, %s\n", endpoint);
            return false;
        }
        *p = '\0'; p += 1;
        int port = atoi(p);
        
        int ret = inet_aton(addr, &pinaddr->sin_addr);
        if ( ret == 0) {
            printf("[ERROR] InetAdderssAdapter::from_string, AF_INET, bad addr, %s", addr);
            return false;
        }
        pinaddr->sin_port = htons(port);
        pinaddr->sin_family = AF_INET;
        
        return true;
    } // end of InetAdderssAdapter::from_string
    
    class Protocol final
    {
    private:
        int m_domain;
        int m_type;
        int m_proto;
        
    public:
        Protocol(int domain, int type, int proto);
        ~Protocol() {}
        
        int domain() const { return m_domain; }
        void domain(int d) { m_domain = d; }
        
        int type() const { return m_type; }
        void type(int t) { m_type = t; }
        
        int protocol() const { return m_proto; }
        void protocol(int p) { m_proto = p; } 
        
        static Protocol tcp4() { return Protocol(AF_INET, SOCK_STREAM, 0); }
        
    }; // end of Protocol 
    
    inline Protocol::Protocol(int domain, int type, int proto)
        : m_domain(domain), m_type(type), m_proto(proto)
    {}
    
    class Socket final
    {
    private:
        Socket(const Socket& ) = delete;
        Socket& operator=(const Socket&) = delete;
        
    private:
        int m_fd;
    
    public:
        Socket(const Protocol &proto);
        ~Socket();
    
        int handle() const { return m_fd; }
        bool bind(const SocketAddress& addr);
        bool listen();
    }; // end of class Socket

    Socket::Socket(const Protocol &proto) 
    {
        m_fd = ::socket(proto.domain(), proto.type(), proto.protocol());
        if ( m_fd < 0 ) {
            printf("[ERROR] Socket::Socket, failed to create socket, %d, %s\n",
                errno, strerror(errno));
        }
    }
    
    Socket::~Socket() 
    {
        if ( m_fd >= 0 ) {
            ::close(m_fd);
            m_fd = -1;
        }
    }
    
    bool Socket::bind(const SocketAddress& addr)
    {
        int ret = ::bind(m_fd, &(const sockaddr&)addr, addr.length());
        if ( ret < 0 ) {
            printf("[ERROR] Socket::bind, bind failed %d, %s\n", errno, strerror(errno));
            return false;
        }
        
        return true;
    } // end of Socket::bind
    
    bool Socket::listen() 
    {
        int ret = ::listen(m_fd, SOMAXCONN);
        if ( ret < 0 ) {
            printf("[ERROR] Socket::listen, listen failed %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;
    } // end of Socket::listen() 
    
    class EPoller final 
    {
    public:
        static const int Event_Read = EPOLLIN;
    
    private:
        int m_epfd;
    
    private:
        EPoller(const EPoller& ) = delete;
        EPoller& operator=(const EPoller&) = delete;
        
    public:
        EPoller();
        ~EPoller();
        
        bool add(int fd, int events, void * pdata);
        bool remove(int fd);
        bool set(int fd, int events, void *pdata);
        
    }; // end of class EPoller
    
    
    inline EPoller::EPoller() 
    {
        m_epfd = ::epoll_create1(EPOLL_CLOEXEC);
        if ( m_epfd < 0 ) {
            printf("[ERROR] EPoller::EPoller, epoll_create1 failed, %d, %s\n", errno, strerror(errno));
        }
    }
    
    inline EPoller::~EPoller()
    {
        if ( m_epfd >= 0 ) {
            ::close(m_epfd);
            m_epfd = -1;
        }
    }
    
    inline bool EPoller::add(int fd, int events, void *pdata)
    {
        epoll_event e;
        e.events = events;
        e.data.ptr = pdata;
        int ret = ::epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &e);
        if ( ret < 0 ) {
            printf("[ERROR] EPoller::add, epoll_ctl failed, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;
    }
    
    inline bool EPoller::set(int fd, int events, void *pdata)
    {
        epoll_event e;
        e.events = events;
        e.data.ptr = pdata;
        int ret = ::epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &e);
        if ( ret < 0 ) {
            printf("[ERROR] EPoller::set, epoll_ctl failed, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;
    }
    
    inline bool EPoller::remove(int fd) 
    {
        int ret = ::epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr);
        if ( ret < 0 ) {
            printf("[ERROR] EPoller::remove, epoll_ctl failed, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;
    }
    
} // end of namespace net 
    
namespace rpc 
{
    class RPC_SocketObject 
    {
    public:
        static const int Type_Listener = 1;
        static const int Type_Channel  = 2;
        
    protected:
        net::Socket m_socket;
        int         m_type;
    public:
        RPC_SocketObject(const net::Protocol& proto, int type) 
            : m_socket(proto), m_type(type) {}
        
        net::Socket& get_socket() { return m_socket; }
        const net::Socket& get_socket() const { return m_socket; }
        
        int type() const { return m_type; }
    }; // end of class RPC_TcpSocketListener
    
    class RPC_TcpSocketChannel {};
    
    class RPC_TcpSocketListener  : public RPC_SocketObject
    {
    public:
        RPC_TcpSocketListener() : RPC_SocketObject(net::Protocol::tcp4(), Type_Listener) {}
    
        bool open(const char * endpoint);
    }; // end of class RPC_TcpSocketListener
    
    template<class Poller = net::EPoller>
    class RPC_Reactor 
    {
    private:
        Poller m_poller;

    public:

        bool reg(RPC_SocketObject *sockobj) 
        {
            bool isok = m_poller.add(sockobj->get_socket().handle(), 0, sockobj);
            if ( !isok ) {
                printf("[ERROR] RPC_Reactor::reg(sockobj) error\n");
                return false;
            }
            return true;
        }
    
        bool add_read(RPC_SocketObject *sockobj, int timeout)
        {
            bool isok = m_poller.set(sockobj->get_socket().handle(), Poller::Event_Read, sockobj);
            if ( !isok ) {
                printf("[ERROR] RPC_Reactor::add_read(sockobj) error\n");
                return false;
            }
            
            // todo timeout queue
            
            return isok;
        }
    
        int run() {
            printf("[ERROR] RPC_Reactor::run error\n");
            return false;
        }
    }; // class RPC_Reactor
    
    class RPC_Message {};
    
    class RPC_TcpSocketService_Impl
    {
    public:
        typedef RPC_TcpSocketChannel   ChannelType;
        typedef RPC_TcpSocketListener  ListenerType;
        typedef RPC_Message            MessageType;
        typedef RPC_Reactor<>          ReactorType;
    }; // end of class RPC_TcpSocketService_Impl
    
    template<class Impl = RPC_TcpSocketService_Impl>
    class RPC_Service
    {
    public:
    
        typedef typename Impl::ChannelType    ChannelType;
        typedef typename Impl::ListenerType   ListenerType;
        typedef typename Impl::ChannelType*   ChannelPtr;
        typedef typename Impl::ListenerType*  ListenerPtr;
        typedef typename Impl::MessageType    MessageType;
        typedef typename Impl::ReactorType    ReactorType;
        
        struct AsyncTask
        {
            int          task_type;
            ChannelPtr   p_channel;
            ListenerPtr  p_listener;
            MessageType  message;
            int          timeout;
        };  // end struct AsyncTask 
        typedef std::queue<AsyncTask*>         AsyncTaskQueue;
        
        static const int Task_AsyncAccept  = 1;
        
    private:
        AsyncTaskQueue m_async_taks_queue;
        ReactorType    m_reactor;
        std::function<void (ListenerPtr, int)> m_accept_error_handler;
        
    private:
        RPC_Service(const RPC_Service&) = delete;
        RPC_Service& operator=(const RPC_Service&) = delete;

    public: 
        RPC_Service();
        ~RPC_Service();
        
        template<class ConnHandler>
        void        set_conn_handle(ConnHandler handler);
        
        template<class RecvHandler>
        void        set_recv_handle(RecvHandler handler);
        
        template<class SendHandler>
        void        set_send_handle(SendHandler handler);
        
        bool        open_channel(const char * endpoint, int timeout);
        bool        close_channel(ChannelPtr channel);
        
        ListenerPtr open_listener(const char * endpoint);
        bool        close_listener(ListenerPtr listener);
        
        bool        post_accept(ListenerPtr listener);
        bool        post_receive(ChannelPtr channel, MessageType & rMessage, int timeout);
        bool        post_send(ChannelPtr channel, MessageType & rMessage, int timeout);
        
        int         run();
    }; // end of class RPC_Service 
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest {
namespace rpc {
    
    bool RPC_TcpSocketListener::open(const char * endpoint)
    {
        net::SocketAddress cSockaddr;
        net::InetAdderssAdapter addr_adapter(cSockaddr);
        bool isok = addr_adapter.from_string(endpoint);
        if ( !isok ) {
            printf("[ERROR] RPC_TcpSocketListener::open, bad endpoint address, %s\n", endpoint);
            return false;
        }
        
        isok = m_socket.bind(cSockaddr);
        if ( !isok ) {
            printf("[ERROR] RPC_TcpSocketListener::open, bind failed, %s\n", endpoint);
            return false;
        }
        
        isok = m_socket.listen();
        if ( !isok ) {
            printf("[ERROR] RPC_TcpSocketListener::open, listen failed, %s\n", endpoint);
            return false;
        }
        printf("[TRACE] RPC_TcpSocketListener::open, result %s\n", isok?"true":"false");
        return isok;
        
    } // end of RPC_TcpSocketListener::open
    
    
    template<class Impl>
    RPC_Service<Impl>::RPC_Service() {}
    
    template<class Impl>
    RPC_Service<Impl>::~RPC_Service() {}
    
    template<class Impl>
    typename RPC_Service<Impl>::ListenerPtr 
    RPC_Service<Impl>::open_listener(const char * endpoint)
    {
        // 打开监听器
        ListenerPtr ptrListener(new ListenerType());
        bool isok = ptrListener->open(endpoint);
        if ( !isok ) {
            printf("[ERROR] RPC_Service::open_listener, open listener failed\n");
            return ListenerPtr(nullptr);
        }
        
        // 注册到Reactor
        isok = m_reactor.reg(ptrListener);
        if ( !isok ) {
            printf("[ERROR] RPC_Service::open_listener, failed reg\n");
            delete ptrListener;
            return ListenerPtr(nullptr);
        }
        
        // 返回监听器指针
        return ptrListener;
    } // end of RPC_Service<Impl>::open_listener
    
    template<class Impl>
    bool RPC_Service<Impl>::post_accept(ListenerPtr listener)
    {
        AsyncTask *task = new AsyncTask;
        
        task->task_type  = Task_AsyncAccept;
        task->p_listener = listener;
        task->timeout    = -1;
        
        // todo: make lock
        m_async_taks_queue.push(task);
        return true;
    }
    
    template<class Impl>
    int RPC_Service<Impl>::run()
    {
        bool isok;
        while ( !m_async_taks_queue.empty() ) {
            AsyncTask * task = m_async_taks_queue.front();
            m_async_taks_queue.pop();
            
            if ( task->task_type == Task_AsyncAccept) {
                m_reactor.add_read(task->p_listener, task->timeout);
            } else {
                printf("[ERROR] RPC_Service::run, unknown task type %d\n", task->task_type);
            }
            delete task;
        }
        
        int ret = m_reactor.run();
        if ( ret < 0 ) {
            printf("[ERROR] RPC_Service::run, reactor run failed\n");
        }
        return ret;
    }
    
} // end of namespace rpc 
} // end of namespace everest



#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
