#ifndef INCLUDE_EVEREST_NET_SOCKET_H
#define INCLUDE_EVEREST_NET_SOCKET_H

#pragma once 
#include <everest/net/sock_addr.h>
#include <fcntl.h>
#include <vector>

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
        Socket(const Protocol &proto, bool create);
        ~Socket();
    
        int  handle() const { return m_fd; }
        void attach(int newfd) { m_fd = newfd; }
        void detach() { m_fd = -1; }
        bool bind(const SocketAddress& addr);
        bool listen();
        bool connect(const SocketAddress& addr);
        bool accept(Socket &sock, SocketAddress &addr);
        
        ssize_t send(std::vector<struct iovec> &buffers);
        
        bool set_block_mode(bool blocked);
        bool set_reuse_addr(bool reuse);
    }; // end of class Socket

    Socket::Socket(const Protocol &proto) 
    {
        m_fd = ::socket(proto.domain(), proto.type(), proto.protocol());
        if ( m_fd < 0 ) {
            printf("[ERROR] Socket::Socket, failed to create socket, %d, %s\n",
                errno, strerror(errno));
        }
    }
    
    Socket::Socket(const Protocol &proto, bool create)
    {
        if ( create ) {
            m_fd = ::socket(proto.domain(), proto.type(), proto.protocol());
            if ( m_fd < 0 ) {
                printf("[ERROR] Socket::Socket, failed to create socket, %d, %s\n",
                    errno, strerror(errno));
            }
        } else {
            m_fd = -1;
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
    
    bool Socket::accept(Socket &sock, SocketAddress &addr)
    {
        int fd = ::accept(m_fd, &(sockaddr &)addr, &addr.length());
        if ( fd < 0 ) {
            printf("[ERROR] Socket::accept, %d, %s\n", errno, strerror(errno));
            return false;
        }
        sock.attach(fd);
        return true;
    }
    
    bool Socket::set_block_mode(bool blocked)
    {
        int flags = ::fcntl(m_fd, F_GETFL);
        if ( flags == -1 ) {
            printf("[ERROR] Socket::set_block_mode, GETFL, %d, %s\n", errno, strerror(errno));
            return false;
        }
        
        int non_block = flags & O_NONBLOCK;
        if ( non_block == 0 && blocked ) return true;   // already blocked
        if ( non_block && !blocked ) return true;   // already non blocked 
          
        flags = ::fcntl(m_fd, F_SETFL, (flags ^= O_NONBLOCK));  // NONBLOCK位异或，0变1,1变0
        if ( flags == -1 ) {
            printf("[ERROR] Socket::set_block_mode, SETFL, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;        
    } // end of Socket::set_block_mode()
    
    bool Socket::set_reuse_addr(bool reuse)
    {
        int val = reuse;
        int ret = ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
        if ( ret < 0 ) {
            printf("[ERROR] Socket::set_resue_addr, %d, %s\n", errno, strerror(errno));
            return false;
        }
        return true;
    }
    
    ssize_t Socket::send(std::vector<struct iovec> &buffers)
    {
        struct msghdr msg;
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        msg.msg_iov = &buffers[0];
        msg.msg_iovlen = buffers.size();
        msg.msg_control = nullptr;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;
    
        ssize_t ret = ::sendmsg(m_fd, &msg, 0);
        printf("[TRACE] Socket::send, iovec\n");
        return ret;
    }
    
} // end of namespace net 
} // end of namespace everest 

#endif // INCLUDE_EVEREST_NET_SOCKET_H
