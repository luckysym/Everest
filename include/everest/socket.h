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
namespace net
{
    /**
     * @brief TcpAddre类，描述TCP网络地址。
     */
    class InetAddress
    {
    private:
        struct sockaddr_in  m_addr;
        
    public:
        InetAddress() {
            m_addr.sin_family = AF_INET;
            m_addr.sin_port = INADDR_ANY;
            m_addr.sin_addr.s_addr = INADDR_ANY;
        }
        
        InetAddress(const char *host, int port) {
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
    }; // end of class InetAddress
    
    /**
     * @brief Socket类。所有Socket的基类，提供基本通用的Socket操作，如句柄管理。
     */
    class BasicSocket
    {
    protected:
        int m_fd;
        
    protected:
        BasicSocket(int fd) : m_fd(fd) {}
        BasicSocket(int family, int type, int proto) {
            m_fd = ::socket(family, type, proto);
        }
        
    public:
        virtual ~BasicSocket() { this->close(); }
        
        bool valid() const { return m_fd >= 0; }
        
        int  handle() const { return m_fd; }
        
        bool close() {
            if ( m_fd >= 0 ) {
                ::close(m_fd);
                m_fd = -1;
            }                
        } // end of close()
    }; // end of class Socket
    
    /**
     * @brief TcpBasicSocket类。使用TCP v4协议的Socket基类，子类包括TcpSocket和TcpServerSocket.
     */
    class TcpBasicSocket : public BasicSocket 
    {
    protected:
        TcpBasicSocket() : BasicSocket(AF_INET, SOCK_STREAM, 0) { }
        TcpBasicSocket(int fd) : BasicSocket(fd) {}
        
    public:
        InetAddress  local_addr() const {
            InetAddress addr;
            socklen_t len = addr.length();
            ::getsockname(m_fd, (struct sockaddr*)addr.c_addr(), &len);
            return addr;
        }
        
        bool bind(const InetAddress &addr) {
            return 0 == ::bind(m_fd, (const struct sockaddr*)addr.c_addr(), addr.length());
        }
    }; // end of class TcpBasicSocket
    
    /**
     * @brief TcpSocket类。支持TCP v4操作。
     */
    class TcpSocket : public TcpBasicSocket
    {
    private:
        InetAddress  m_raddr;
        
    public:
        TcpSocket() { }
        
        TcpSocket(int fd) : TcpBasicSocket(fd) { }
        
        InetAddress  remote_addr() const { return m_raddr; }
        
        bool open(const InetAddress & remote) {
            m_raddr = remote;
            return 0 == ::connect(m_fd, (const struct sockaddr*)remote.c_addr(), remote.length());
        }
        
        bool attach(int fd, const InetAddress & raddr) {
            this->close();
            m_fd = fd;
            m_raddr = raddr;
            return true;
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
        
        bool open(int backlog) {
            return 0 == ::listen(m_fd, backlog);
        }
        
        int  accept( InetAddress& addr) {
            socklen_t len = addr.length();
            return ::accept(m_fd, (struct sockaddr*)addr.c_addr(), &len);
        }
        
        bool accept(TcpSocket & sock) {
            InetAddress addr;
            socklen_t len = addr.length();
            int fd = ::accept(m_fd, (struct sockaddr*)addr.c_addr(), &len );
            if ( fd >= 0 ) {
                sock.attach(fd, addr);
                return true;
            } else {
                return false;
            }
        }
    }; // end of class TcpServerSocket
    
} // end of namespace net
} // end of namespace everest

#endif // INCLUDED_EVEREST_SOCKET_H
