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

#include <errno.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>


namespace everest
{
namespace net 
{
    inline socklen_t SOCK_Set_Address_From_String(
        sockaddr *addr, socklen_t len, int af, const char *str, int port)
    {
        if ( af == AF_INET ) {
            sockaddr_in * pinaddr = (sockaddr_in*)addr;
            if ( sizeof(sockaddr_in) > len ) {
                printf("[ERROR] SOCK_Set_Address_From_String, AF_INET, no enough space");
                return false;
            }
            pinaddr->sin_family = af;
            pinaddr->sin_port = htons(port);
            int ret = inet_aton(str, &pinaddr->sin_addr);
            if ( ret == 0) {
                printf("[ERROR] SOCK_Set_Address_From_String, AF_INET, bad addr, %s", str);
                return false;
            }
            return true;
        } else {
            printf("[ERROR] bad address family %d\n", af);
            return false;
        }
    } // end of SOCK_Set_Address_From_String
    
    class Socket final
    {
    private:
        Socket(const Socket& ) = delete;
        Socket& operator=(const Socket&) = delete;
        
    private:
        int m_af;
        int m_fd;
    
    public:
        Socket() {}
        ~Socket() {}
    
        bool bind(const char *addr, int port);
        bool listen();
        bool close();
    }; // end of class Socket

    bool Socket::bind(const char *addr, int port)
    {
        char addrbuf[128];
        socklen_t len = SOCK_Set_Address_From_String((sockaddr *)addrbuf, 128, m_af, addr, port);
        if ( len == (socklen_t)-1 ) {
            printf("[ERROR] Socket::bind, set address failed \n");
            return false;
        }
        int ret = ::bind(m_fd, (const sockaddr*)addrbuf, len);
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
    
} // end of namespace net 
    
namespace rpc 
{
    class RPC_TcpSocketChannel {};
    
    class RPC_TcpSocketListener 
    {
    private:
        net::Socket m_socket;
        
    public:
        bool open(const char * endpoint);
    }; // end of class RPC_TcpSocketListener
    
    class RPC_TcpSocketService_Impl
    {
    public:
        typedef RPC_TcpSocketChannel   ChannelType;
        typedef RPC_TcpSocketListener  ListenerType;
    }; // end of class RPC_TcpSocketService_Impl
    
    class RPC_Message {};
    
    template<class Impl = RPC_TcpSocketService_Impl>
    class RPC_Service
    {
    public:
        typedef typename Impl::ChannelType                   ChannelType;
        typedef typename Impl::ListenerType                  ListenerType;
        typedef std::shared_ptr<typename Impl::ChannelType>  ChannelPtr;
        typedef std::shared_ptr<typename Impl::ListenerType> ListenerPtr;
        typedef std::shared_ptr<RPC_Message>                 MessagePtr;
        
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
        bool        post_receive(ChannelPtr channel, MessagePtr ptrMessage, int timeout);
        bool        post_send(ChannelPtr channel, MessagePtr ptrMessage, int timeout);
        
        int         run();
        
    }; // end of class RPC_Service 
        
} // end of namespace rpc 
} // end of namespace everest 

namespace everest {
namespace rpc {
    
    bool RPC_TcpSocketListener::open(const char * endpoint)
    {
        char * addr = ::strdup(endpoint);
        char * p = strchr(addr, ':');
        if ( p == nullptr ) {
            free(addr);
            printf("[ERROR] RPC_TcpSocketListener::open, bad endpoint, %s\n", endpoint);
            return false;
        }
        *p = '\0'; p += 1;
        
        int port = atoi(p);
        bool isok = m_socket.bind(addr, port);
        free(addr);
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
        ListenerPtr ptrListener(new ListenerType());
        bool isok = ptrListener->open(endpoint);
        if ( isok ) return ptrListener;
        else return ListenerPtr();
    } // end of RPC_Service<Impl>::open_listener
    
    template<class Impl>
    bool RPC_Service<Impl>::post_accept(ListenerPtr listener)
    {
        return false;
    }
    
    template<class Impl>
    int RPC_Service<Impl>::run()
    {
        return -1;
    }
    
} // end of namespace rpc 
} // end of namespace everest



#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
