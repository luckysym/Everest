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
        TcpAddress() {
            m_addr.sin_family = AF_INET;
            m_addr.sin_port = INADDR_ANY;
            m_addr.sin_addr.s_addr = INADDR_ANY;
        }
        
        TcpAddress(const char *host, int port) {
            m_addr.sin_family = AF_INET;
            m_addr.sin_port = htons(port);
            if ( host == nullptr || host[0] == '\0')  
                m_addr.sin_addr.s_addr = INADDR_ANY;
            else {
                int ret = ::inet_pton(AF_INET, host, &m_addr.sin_addr);
                assert(  ret == 1 );
            }
        }
        
        struct sockaddr_in * c_addr() { return &m_addr;}
        const struct sockaddr_in * c_addr() const { return &m_addr;}
        
        size_t length() const { return sizeof(m_addr);}
    }; // end of class TcpAddress
    
    /**
     * @brief Socket类。所有Socket的基类，提供基本通用的Socket操作，如句柄管理。
     */
    class BasicSocket
    {
    protected:
        int m_fd;
        
    protected:
        BasicSocket() : m_fd(-1) {}
        BasicSocket(int fd) : m_fd(fd) {}
        
    public:
        virtual ~BasicSocket() { 
            this->close();
        }
        
        bool valid() const { return m_fd >= 0; }
        
        bool close() {
            if ( m_fd >= 0 ) {
                ::close(m_fd);
                m_fd = -1;
            }                
        } // end of close()
    }; // end of class Socket
    
    class TcpBasicSocket : public BasicSocket 
    {
    protected:
        TcpBasicSocket() {
            m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        }
        
        TcpBasicSocket(int fd) : BasicSocket(fd) {}
    }; // end of class TcpBasicSocket
    
    /**
     * @brief TcpSocket类。支持TCP v4操作。
     */
    class TcpSocket : public TcpBasicSocket
    {
    public:
        TcpSocket() { }
        
        TcpSocket(int fd) : TcpBasicSocket(fd) { }
        
        bool open(const TcpAddress & remote) {
            return 0 == ::connect(m_fd, (const struct sockaddr*)remote.c_addr(), remote.length());
        }
    
        ssize_t send(const char *buf, size_t len) {
            return ::send(m_fd, buf, len, 0);
        }
        
        ssize_t recv(char * buf, size_t len) {
            return ::recv(m_fd, buf, len, 0);
        }
    }; // end of class TcpSocket
    
    class TcpServerSocket : public TcpBasicSocket
    {
    public:
        TcpServerSocket() { }
    
        bool bind(const TcpAddress &addr) {
            return 0 == ::bind(m_fd, (const struct sockaddr*)addr.c_addr(), addr.length());
        }
        
        bool open(int backlog) {
            return 0 == ::listen(m_fd, backlog);
        }
        
        int  accept( TcpAddress& addr) {
            socklen_t len = addr.length();
            return ::accept(m_fd, (struct sockaddr*)addr.c_addr(), &len);
        }
    }; // end of class TcpServerSocket
    
} // end of namespace sock
} // end of namespace everest

#endif // INCLUDED_EVEREST_SOCKET_H
