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
        static const int Event_None = 0;
        static const int Event_Read = EPOLLIN;
        
        static const int Step_Size = 1024;
        
        class Event 
        {
        public:
            
            int fd() {
                printf("[ERROR] EPoller::Event::fd, not impl\n");
                return -1;
            }
            
            void * data() { 
                printf("[ERROR] EPoller::Event::data, not impl\n");
                return nullptr; 
            }
            
            int events() {
                printf("[ERROR] EPoller::Event::events, not impl\n");
                return 0;
            }
        }; // end of Event 
        
        class Iterator 
        {
        public:
            bool has_next() { 
                printf("[ERROR] EPoller::Iterator::has_next, not impl\n");
                return false; 
            }
            
            Event next() { 
                printf("[ERROR] EPoller::Iterator::next, not impl\n");
                return Event();
            }
                        
        }; // end of Iterator 
        
    private:
        int           m_epfd;
        int           m_maxevents;
        int           m_eventcount;
        epoll_event * m_pevents;
        
    private:
        EPoller(const EPoller& ) = delete;
        EPoller& operator=(const EPoller&) = delete;
        
    public:
        EPoller();
        ~EPoller();
        
        bool add(int fd, int events, void * pdata);
        bool remove(int fd);
        bool set(int fd, int events, void *pdata);
        int  wait(int timeout);
        
        Iterator events() {
            printf("[ERROR] EPoller::events, not impl\n");
            return Iterator();
        }
    }; // end of class EPoller
    
    
    inline EPoller::EPoller() : m_maxevents(0), m_eventcount(0)
    {
        m_epfd = ::epoll_create1(EPOLL_CLOEXEC);
        if ( m_epfd < 0 ) {
            printf("[ERROR] EPoller::EPoller, epoll_create1 failed, %d, %s\n", errno, strerror(errno));
        }
        m_pevents = (epoll_event *)malloc(Step_Size * sizeof(epoll_event));
        m_maxevents = Step_Size;
        assert(m_pevents);
    }
    
    inline EPoller::~EPoller()
    {
        if ( m_epfd >= 0 ) {
            ::close(m_epfd);
            m_epfd = -1;
        }
        free(m_pevents);
        m_pevents = nullptr;
        m_eventcount = 0;
        m_maxevents = 0;
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
        m_eventcount += 1;
        if ( m_eventcount > m_maxevents ) {
            epoll_event * pevents = (epoll_event*)::realloc(m_pevents, (m_maxevents + Step_Size) * sizeof(epoll_event));
            assert(pevents);
            m_pevents = pevents;
            m_maxevents += Step_Size;
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
    
    inline int EPoller::wait(int timeout) 
    {
        int ret = ::epoll_wait(m_epfd, m_pevents, m_maxevents, timeout);
        if ( ret < 0 ) {
            printf("[ERROR] EPoller::wait, epoll_wait fail, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return ret;
    }
    
} // end of namespace net 
    
namespace rpc 
{
    /**
     * RPC过程常量集合
     */
    struct RPC_Constants
    {
        static const int Finish = 0;
    }; // end of RPC_Constants
    
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
    
    /**
     * 超时队列
     */
    class RPC_TimeoutQueue
    {
    public:
        int front() const { return 0; }
        void pop_front() {}
    }; // end of class RPC_TimeoutQueue
    
    template<class Poller = net::EPoller, class TimeoutQ = RPC_TimeoutQueue>
    class RPC_Proactor 
    {
    private:
        Poller   m_poller;
        TimeoutQ m_timeout_queue;   // 超时队列
        
    public:

        bool reg(RPC_SocketObject *sockobj) 
        {
            bool isok = m_poller.add(sockobj->get_socket().handle(), 0, sockobj);
            if ( !isok ) {
                printf("[ERROR] RPC_Proactor::reg(sockobj) error\n");
                return false;
            }
            return true;
        }
    
        bool add_read(RPC_SocketObject *sockobj, int timeout)
        {
            bool isok = m_poller.set(sockobj->get_socket().handle(), Poller::Event_Read, sockobj);
            if ( !isok ) {
                printf("[ERROR] RPC_Proactor::add_read(sockobj) error\n");
                return false;
            }
            
            // todo timeout queue
            
            return isok;
        }
    
        int run() {
            int timeout = m_timeout_queue.front();
            m_timeout_queue.pop_front();
            int ret = m_poller.wait(timeout);
            if ( ret > 0 ) {
                // 有事件发生
                typename Poller::Iterator iter = m_poller.events();
                while ( iter.has_next() ) {
                    typename Poller::Event e = iter.next();
                    if ( e.events() & Poller::Event_Read ) {
                        int ret = this->on_readable((RPC_SocketObject*)e.data());
                        if ( ret == RPC_Constants::Finish ) {
                            // 当前接收任务结束（比如本次消息完整接收），就暂停监听该socket
                            m_poller.set(e.fd(), Poller::Event_None, nullptr);
                        } else {
                            throw std::runtime_error("RPC_Proactor::run, unknown callback returned value");
                        }
                    }
                } // end while
            } else if ( ret == 0 ) {
                // 超时，并没有事件发生
                printf("[ERROR] RPC_Proactor::run, poller wait timeout\n");
            } else {
                // poller wait出现错误
                printf("[ERROR] RPC_Proactor::run, poller wait error\n");
            }
            return ret;
            
            return false;
        }
    private:
        int on_readable(RPC_SocketObject * psockobj) {
            printf("[ERROR] RPC_Proactor::on_readable, poller wait error\n");
            return RPC_Constants::Finish;
        }
    }; // class RPC_Proactor
    
    class RPC_Message {};
    
    class RPC_TcpSocketService_Impl
    {
    public:
        typedef RPC_TcpSocketChannel   ChannelType;
        typedef RPC_TcpSocketListener  ListenerType;
        typedef RPC_Message            MessageType;
        typedef RPC_Proactor<>         ProactorType;
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
        typedef typename Impl::ProactorType   ProactorType;
        
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
        ProactorType   m_proactor;
        
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
    RPC_Service<Impl>::RPC_Service() 
    {
    }
    
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
        isok = m_proactor.reg(ptrListener);
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
                m_proactor.add_read(task->p_listener, task->timeout);
            } else {
                printf("[ERROR] RPC_Service::run, unknown task type %d\n", task->task_type);
            }
            delete task;
        }
        
        int ret = m_proactor.run();
        if ( ret < 0 ) {
            printf("[ERROR] RPC_Service::run, reactor run failed\n");
        }
        return ret;
    }
    
} // end of namespace rpc 
} // end of namespace everest



#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
