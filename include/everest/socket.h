#ifndef INCLUDED_EVEREST_SOCKET_H
#define INCLUDED_EVEREST_SOCKET_H

#pragma once 

#include <assert.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace everest
{
namespace sock
{
    /**
     * @brief TcpAddre类，描述TCP网络地址。
     */
    class TcpAddress
    {
    private:
        struct sockaddr_in  m_addr;
        
    public:
        TcpAddress(const char *host, int port) {
            m_addr.sin_family = AF_INET;
            m_addr.sin_port = htons(port);
            int ret = ::inet_pton(AF_INET, host, &m_addr.sin_addr);
            assert(  ret == 1 );
        }
        
        struct sockaddr_in * c_addr() { return &m_addr;}
        const struct sockaddr_in * c_addr() const { return &m_addr;}
        
        size_t length() const { return sizeof(m_addr);}
    }; // end of class TcpAddress
    
    
    /**
     * @brief TcpSocket类。支持TCP v4操作。
     */
    class TcpSocket 
    {
    private:
        int m_fd;
        
    public:
        TcpSocket() {
            m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        }
    
        ssize_t send(const char *buf, size_t len) {
            return ::send(m_fd, buf, len, 0);
        }
        
        ssize_t recv(char * buf, size_t len) {
            return ::recv(m_fd, buf, len, 0);
        }
        
        bool open(const TcpAddress & remote) {
            return 0 == ::connect(m_fd, (const struct sockaddr*)remote.c_addr(), remote.length());
        }
        
        bool close() { return 0 == ::close(m_fd); }
    }; // end of class TcpSocket
    
} // end of namespace sock
} // end of namespace everest

#endif // INCLUDED_EVEREST_SOCKET_H
