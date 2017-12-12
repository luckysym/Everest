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
        static const int Finish = 0;
        
        static const int Read   = 1;
        static const int Write  = 2;
        static const int Accept = 4;
        
        static const int64_t Max_Expire_Time = INT64_MAX;
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
    
    class RPC_SocketListener : public RPC_SocketObject
    {
    public:
        RPC_SocketListener(const net::Protocol& proto) 
            : RPC_SocketObject(proto, Type_Listener) 
        {}
    };
    
    class RPC_SocketChannel : public RPC_SocketObject 
    {
    public: 
        RPC_SocketChannel(const net::Protocol& proto)
            : RPC_SocketObject(proto, Type_Channel)
        {}
    };
    
    class RPC_TcpSocketChannel : public RPC_SocketChannel 
    {
    public:
        RPC_TcpSocketChannel()
            : RPC_SocketChannel(net::Protocol::tcp4())
        {}
        
        bool open(const char *endpoint);
    };
    
    class RPC_TcpSocketListener  : public RPC_SocketListener
    {
    public:
        RPC_TcpSocketListener() 
            : RPC_SocketListener(net::Protocol::tcp4()) 
        {}
    
        bool open(const char * endpoint);
    }; // end of class RPC_TcpSocketListener
    
    
////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION 
        
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
    
    bool RPC_TcpSocketChannel::open(const char * endpoint)
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
    
} // end of namespace rpc 
} // end of namespace everest 

#endif // INCLUDE_EVEREST_RPC_RPC_SOCKET_H
