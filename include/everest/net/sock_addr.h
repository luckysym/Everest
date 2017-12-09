#ifndef INCLUDE_EVEREST_NET_SOCK_ADDR_H
#define INCLUDE_EVEREST_NET_SOCK_ADDR_H

#pragma once 

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
    
} // end of namespace net 
} // end of namespace everest 


#endif // INCLUDE_EVEREST_NET_SOCK_ADDR_H
