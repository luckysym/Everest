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
        static const int Event_Write = EPOLLOUT;
        
        static const int Step_Size = 1024;
        
        class Event 
        {
        private:
            const epoll_event * m_pevent;
            
        public:
            Event(const epoll_event * pevent) : m_pevent(pevent) {}
            
            int events() const { return m_pevent->events; }
            
            void * data() { return m_pevent->data.ptr; }
            
            int fd () const { m_pevent->data.fd; }
        }; // end of Event 
        
        class Iterator 
        {
        private:
            epoll_event * const m_pevents;
            int                 m_count;
            int                 m_cursor;
        public:
            Iterator(epoll_event * const pevents, int count)
                : m_pevents(pevents), m_count(count), m_cursor(0)
            {
                printf("[TRACE] EPoller::Iterator(), events count %d\n", count);
            }
        
            bool has_next() const { return m_cursor < m_count; }
            
            Event next() { 
                return Event(&m_pevents[m_cursor++]);
            }
            
        }; // end of Iterator 
        
    private:
        int           m_epfd;
        int           m_maxevents;
        int           m_eventcount;
        epoll_event * m_pevents;
        int           m_count;
        
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
            return Iterator(m_pevents, m_count);
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
            m_count = 0;
            return false;
        }
        m_count = ret;
        return ret;
    }
    
} // end of namespace net 
} // end of namespace everest 


#endif // INCLUDE_EVEREST_NET_EPOLLER_H