#ifndef INCLUDE_EVEREST_NET_EPOLLER_H
#define INCLUDE_EVEREST_NET_EPOLLER_H

#pragma once 

#include <sys/types.h>
#include <sys/epoll.h>
#include <assert.h>

namespace everest
{
namespace net 
{
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
} // end of namespace everest 


#endif // INCLUDE_EVEREST_NET_EPOLLER_H