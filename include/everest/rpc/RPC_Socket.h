#ifndef INCLUDE_EVEREST_RPC_RPC_SOCKET_H
#define INCLUDE_EVEREST_RPC_RPC_SOCKET_H

#pragma once 

#include <stdexcept>

namespace everest
{
namespace rpc 
{
    /**
     * RPC过程常量集合
     */
    struct RPC_Constants
    {
        static const int Ok     = 0;
        static const int Fail   = -1;
        
        static const int Read   = 1;
        static const int Write  = 2;
        static const int Accept = 4;
        
        static const int64_t Max_Expire_Time = INT64_MAX;
    }; // end of RPC_Constants
    
    class RPC_SocketChannel;
    
    class RPC_SocketObject 
    {
    public:
        static const int Type_Listener = 1;
        static const int Type_Channel  = 2;
        
    protected:
        net::Protocol      m_proto;
        net::Socket        m_socket;
        net::SocketAddress m_addr;
        int                m_type;

    public:
        RPC_SocketObject(const net::Protocol& proto, int type) 
            : m_proto(proto), m_socket(proto), m_type(type) {}
        
        RPC_SocketObject(const net::Protocol& proto, int type, 
                         net::Socket &sock, net::SocketAddress &addr) 
            : m_proto(proto), m_socket(proto, false), m_type(type), m_addr(addr)
        {
            m_socket.attach(sock.handle());
            sock.detach();
        }

        net::Socket& get_socket() { return m_socket; }
        
        const net::Socket& get_socket() const { return m_socket; }
        
        int type() const { return m_type; }
        
    }; // end of class RPC_TcpSocketListener
    
    class RPC_SocketChannel : public RPC_SocketObject 
    {
    public: 
        RPC_SocketChannel()
            : RPC_SocketObject(net::Protocol::tcp4(), Type_Channel)
        {}
        
        RPC_SocketChannel(net::Socket &sock, net::SocketAddress &addr)
            : RPC_SocketObject(net::Protocol::tcp4(), Type_Channel, sock, addr)
        {}
        
        bool open(const char * endpoint);
    };
    
    class RPC_SocketListener : public RPC_SocketObject
    {
    public:
        RPC_SocketListener() 
            : RPC_SocketObject(net::Protocol::tcp4(), Type_Listener) 
        {}
        
        bool                open(const char * endpoint);
        RPC_SocketChannel * accept();
    };
    
    
////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION 
        
    inline 
    bool RPC_SocketListener::open(const char * endpoint)
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
        
    } // end of RPC_SocketListener::open
    
    inline 
    bool RPC_SocketChannel::open(const char * endpoint)
    {
        net::SocketAddress addr;
        net::InetAdderssAdapter addr_adapter(addr);
        bool isok = addr_adapter.from_string(endpoint);
        if ( !isok ) {
            printf("[ERROR] RPC_TcpSocketChannel::open, bad endpoint address, %s\n", endpoint);
            return false;
        }
        isok = m_socket.connect(addr);
        if ( !isok ) {
            printf("[ERROR] RPC_TcpSocketChannel::open, connect failed, %s\n", endpoint);
            return false;
        }
        return true;
    }
    
    inline 
    RPC_SocketChannel * RPC_SocketListener::accept()
    {
        net::Socket        newsock(m_proto, false);  // 仅仅建一个空的socket
        net::SocketAddress addr;
        
        bool isok = m_socket.accept(newsock, addr);
        if ( !isok ) {
            printf("[ERROR] RPC_SocketListener::accept failed\n");
            return nullptr;
        }
        printf("[ERROR] RPC_SocketListener::accept, new channel accepted, fd %d\n");
        return (RPC_SocketChannel *) new RPC_SocketChannel(newsock, addr);
    }
    
} // end of namespace rpc 
} // end of namespace everest 

#endif // INCLUDE_EVEREST_RPC_RPC_SOCKET_H
