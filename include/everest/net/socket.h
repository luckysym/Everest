#ifndef INCLUDE_EVEREST_NET_SOCKET_H
#define INCLUDE_EVEREST_NET_SOCKET_H

#pragma once 
#include <everest/net/sock_addr.h>

namespace everest
{
namespace net 
{
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
        bool connect(const SocketAddress& addr);
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
    
    bool Socket::connect(const SocketAddress& addr) 
    {
        int ret = ::connect(m_fd, &(const sockaddr&)addr, addr.length());
        if ( ret == 0 ) return true;
        else if ( ret < 0 ) {
            if ( errno == EINPROGRESS ) {
                printf("[INFO] Socket::connect, conn in progress\n");
                return true;
            } else {
                printf("[ERROR] Socket::connect, conn ok, %d, %s\n", errno, strerror(errno));
                return false;
            }
        }
    }
    
} // end of namespace net 
} // end of namespace everest 

#endif // INCLUDE_EVEREST_NET_SOCKET_H
