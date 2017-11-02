#ifndef INCLUDED_EVEREST_NETWORK_H
#define INCLUDED_EVEREST_NETWORK_H

#pragma once 
#include <sys/epoll.h>
#include <vector>

namespace everest
{
namespace net
{
    /// EPollEvent类。epoll_event封装
    class EPollEvent : public epoll_event
    {
    public:
        EPollEvent() {}
        
        void reset () {
            epoll_event::events = 0;
            epoll_event::data.ptr = nullptr;
        }
        
        uint32_t events() const { return epoll_event::events;   }
        void *   object() const   { return epoll_event::data.ptr; }
        
    }; // end of class EPollEvent
    
    /**
     * @brief EPollerIterator类。Epoller事件迭代器。
     */
    class EPollIterator : public std::vector<EPollEvent>::iterator
    {
    public:
        EPollIterator() {}
        
        EPollIterator(const std::vector<EPollEvent>::iterator & it) 
            : std::vector<EPollEvent>::iterator(it)
        {}
    }; // end of class EPollIterator
    
    /**
     * @brief EPoller类。Linux epoll操作类。
     */
    class EPoller
    {
    public:
        typedef EPollIterator Iterator;
        
    private:
        int    m_epfd; 
        int    m_event_count;
        std::vector<EPollEvent> m_events;
        std::vector<EPollEvent> m_revents;
        
    public:
        EPoller() : m_event_count(0) {
            EPollEvent e;
            e.reset();
            
            m_epfd = ::epoll_create1(EPOLL_CLOEXEC);
            m_events.resize(32, e);
            m_revents.resize(32, e);
        }
        
        ~EPoller() { this->close(); }
    
        bool close() { 
            if ( m_epfd >= 0 ) {
                ::close(m_epfd);
                m_epfd = -1;
            }
            return true;
        }
    
        bool add(int fd, int32_t events, void *param) {
            // 扩大设置事件空间，当events数组长度不足时。
            if ( fd >= m_events.size() ) {
                EPollEvent e;
                e.reset();
                m_events.resize(fd + 32, e);
            }
            m_events[fd].data.ptr = param;
            m_events[fd].epoll_event::events = events;
            int ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &m_events[fd]);
            if ( ret != 0 ) {
                m_events[fd].data.ptr = nullptr;
                return false;
            } else {
                ++m_event_count;
                return true;
            }
        } // end of add()
        
        int  wait(int timeout) {
            EPollEvent e;
            e.reset();
            m_revents.resize(m_event_count, e);
            int ret = ::epoll_wait(m_epfd, &m_revents[0], m_event_count, timeout);
            if ( ret >= 0 ) m_revents.resize(ret, e);
            return ret;
        }
        
        bool remove(int fd) {
            if ( m_events[fd].object() != nullptr ) {
                int ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr);
                m_events[fd].data.ptr = nullptr;
                --m_event_count;
                return ret == 0;
            }
            return true;
        }
        
        /// 获取第一个设置的事件
        Iterator begin() {
            std::vector<EPollEvent>::iterator it = m_events.begin();
            for( ; it != m_events.end(); ++it) {
                if ( it->data.ptr != nullptr ) {
                    return Iterator(it);
                }
            }
            return Iterator(it);
        }
        
        /// 获取设置的事件列表末尾迭代器
        Iterator end() {
            return Iterator(m_events.end());
        }
        
        /// 获取第一个触发的事件迭代器。
        Iterator rbegin() {
            return Iterator(m_revents.begin());
        }
        
        /// 获取触发的事件列表的末尾
        Iterator rend() {
            return Iterator(m_revents.end());
        }
        
    }; // end of class EPoller 
    
} // end of namespace net
} // end of namespace everest


#endif // INCLUDED_EVEREST_NETWORK_H