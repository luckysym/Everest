#ifndef INCLUDE_EVEREST_RPC_RPC_PROACTOR_H
#define INCLUDE_EVEREST_RPC_RPC_PROACTOR_H

#pragma once 
#include <queue>
#include <map>
#include <unordered_map>
#include <functional>

#include <everest/rpc/RPC_Message.h>

namespace everest 
{
namespace rpc 
{
    /**
     * 超时队列
     */
    class RPC_TaskTimeoutQueue
    {
    public:
        class TaskOwner;
    
        class Task 
        {
        private:
            TaskOwner * m_owner_ref;
            int         m_type;
            int64_t     m_expire_time;
            RPC_Message m_msg;
            
        public:
            Task(TaskOwner *owner, int type, RPC_Message& msg, int64_t exp) 
                : m_owner_ref(owner), m_type(type), m_expire_time(exp), m_msg(msg) {}
        
            int64_t expire_time() const {  return m_expire_time; }
            
            int type() const { return m_type; }
            
            TaskOwner * owner() { return m_owner_ref; }
            
            RPC_Message & message() { return m_msg; }
        };
        
        class TaskOwner 
        {
        private:
            RPC_SocketObject * m_sock_ref;
            std::queue<Task> m_wr_queue;  // 读任务队列
            std::queue<Task> m_rd_queue;  // 写任务队列
            
        public:
            TaskOwner(RPC_SocketObject* p) : m_sock_ref(p) {}
        
            RPC_SocketObject * get_socket() const { return m_sock_ref; }
        
            Task * get_front_task(int type) 
            {
                std::queue<Task> * queue = nullptr;
                if ( type == RPC_Constants::Read ) queue = &m_rd_queue;
                else if ( type == RPC_Constants::Write) queue = &m_wr_queue;
                else {
                    printf("[WARN] RPC_TaskTimeoutQueue::TaskOwner::get_front_task, wrong type\n");
                    return nullptr;
                }
                
                printf("[TRACE] RPC_TaskTimeoutQueue::TaskOwner::get_front_task, type %d, empty %s\n", 
                    type, queue->empty()?"true":"false");
                
                if ( !queue->empty() ) return &queue->front();
                else {
                    printf("[WARN] RPC_TaskTimeoutQueue::TaskOwner::get_front_task, no task\n");
                    return nullptr;
                }
            }
            
            void pop_front_task(int type)
            {
                std::queue<Task> * queue = nullptr;
                if ( type == RPC_Constants::Read ) queue = &m_rd_queue;
                else if ( type == RPC_Constants::Write) queue = &m_wr_queue;
                else {
                    printf("[ERROR] RPC_TaskTimeoutQueue::TaskOwner::pop_front_task, wrong type\n");
                }
                
                printf("[TRACE] RPC_TaskTimeoutQueue::TaskOwner::pop_front_task, type %d, empty %s\n", 
                    type, queue->empty()?"true":"false");
                
                if ( !queue->empty() ) queue->pop();
                else {
                    printf("[ERROR] RPC_TaskTimeoutQueue::TaskOwner::pop_front_task, no task\n");
                }
            }
            
            Task * push_task(const Task &task) 
            {
                printf("[TRACE] RPC_TaskTimeoutQueue::TaskOwner::push_task, task type %d\n", task.type());
                int type = task.type();
                if ( type == RPC_Constants::Read ) {
                    m_rd_queue.push(task);
                    return &m_rd_queue.back();
                } else if ( type == RPC_Constants::Write ) {
                    m_wr_queue.push(task);
                    return &m_wr_queue.back();
                } else {
                    printf("[WARN] RPC_TaskTimeoutQueue::TaskOwner::push_task, wrong task type %d\n", type);
                    return nullptr;
                }
            } // end of push_task
        }; // end of class TaskOwner
        
    private:
        typedef std::multimap<int64_t, Task *>  TimeoutTaskMap;
        typedef std::unordered_map<RPC_SocketObject *, TaskOwner*> TaskOwnerMap;
        
    private:
        TimeoutTaskMap  m_timeout_map;
        TaskOwnerMap    m_owner_map;
        
    public:
    
        bool empty() const {
            return m_timeout_map.empty();
        }
        
        TaskOwner * add_owner(RPC_SocketObject * p) {
            TaskOwnerMap::iterator it = m_owner_map.find(p);
            if ( it == m_owner_map.end() ) {
                TaskOwner * owner = new TaskOwner(p);
                std::pair<TaskOwnerMap::iterator, bool > result 
                    = m_owner_map.insert(TaskOwnerMap::value_type(p, owner));
                if ( result.second ) return owner;
                else {
                    printf("[ERROR] RPC_TaskTimeoutQueue::add_owner, add failed\n");
                    return nullptr;
                }
            } else {
                return it->second;
            }
        }
        
        TaskOwner * find_owner(RPC_SocketObject * p) {
            TaskOwnerMap::iterator it = m_owner_map.find(p);
            if ( it != m_owner_map.end() ) {
                return it->second;
            } else {
                printf("[ERROR] RPC_TaskTimeoutQueue::find_owner, not found\n");
                return nullptr;
            }
        }
        
        Task * push_task(TaskOwner * owner, int type, RPC_Message &msg, int64_t expire) 
        {
            // 先写入owner任务队列
            Task * p_task = owner->push_task(Task(owner, type, msg, expire));
            if ( !p_task ) {
                printf("[ERROR] RPC_TaskTimeoutQueue::push_task, push task fail\n");
                return nullptr;
            }
            
            printf("[TRACE] RPC_TaskTimeoutQueue::push_task, insert timeout map\n");
            // 再写入timeout队列
            TimeoutTaskMap::iterator it = m_timeout_map.insert(TimeoutTaskMap::value_type(expire, p_task));
            assert(it != m_timeout_map.end());
            
            return p_task;
        } // end of push_task
        
        Task * get_front_timeout() 
        {
            if ( !m_timeout_map.empty() ) {
                return m_timeout_map.begin()->second;
            } else {
                printf("[ERROR] RPC_TaskTimeoutQueue::get_front_timeout, no front timeout task\n");
                return nullptr;
            }
        }  // end of get_front_timeout
        
        void pop_front_timeout() 
        {
            throw std::runtime_error("RPC_TaskTimeoutQueue::pop_front_timeout not impl");
        }
    }; // end of class RPC_TimeoutQueue
    
    template<class Poller = net::EPoller>
    class RPC_Proactor 
    {
    private:
        Poller               m_poller;
        RPC_TaskTimeoutQueue m_task_timeout_queue;   // 任务超时队列
        
        std::function<int (RPC_SocketListener*, RPC_SocketChannel*, int ec)> m_accept_handler;
        std::function<int (RPC_SocketChannel*, int ec)> m_connect_handler;  // channel连接成功处理
        std::function<int (RPC_SocketChannel*, RPC_Message &, int ec)> m_send_handler;  // 发送成功回调
        
        std::vector<struct iovec> m_send_iovec;
        
    public:
        RPC_Proactor() {
            m_send_iovec.reserve(16);
        }
        
        
        template<class Handler>
        void set_accept_handler(const Handler &handler) { m_accept_handler = handler; }
        
        template<class Handler>
        void set_connect_handler(const Handler &handler) { m_connect_handler = handler; }
    
        template<class Handler>
        void set_send_handler(const Handler &handler) { m_send_handler = handler; }
    
        bool reg(RPC_SocketObject *sockobj) 
        {
            // 任务超时队列中注册socket对象
            RPC_TaskTimeoutQueue::TaskOwner * p_owner = m_task_timeout_queue.add_owner(sockobj);
            assert(p_owner);
            
            bool isok = m_poller.add(sockobj->get_socket().handle(), 0, p_owner);
            if ( !isok ) {
                printf("[ERROR] RPC_Proactor::reg(sockobj) error\n");
                return false;
            }
            return true;
        }

        bool add_read(RPC_SocketObject *sockobj, RPC_Message &msg, int64_t expire);
        
        bool add_write(RPC_SocketObject *sockobj, RPC_Message &msg, int64_t expire);

        int run_once() {
            int64_t now = DateTime::get_timestamp();
            if ( m_task_timeout_queue.empty() ) {
                printf("[INFO] RPC_Proactor::run, no task \n");
                return 0;
            }
            
            bool  need_clear_timeout = false;   // 是否在wait后需要清理超时任务
            
            RPC_TaskTimeoutQueue::Task * p_task = m_task_timeout_queue.get_front_timeout();
            int timeout = (int)((p_task->expire_time() - now) / 1000);
            if ( timeout < 0 ) {
                need_clear_timeout = true;
                timeout = 0;
            }
            int ret = m_poller.wait(timeout);
            if ( ret > 0 ) {
                this->process_events();
                if ( need_clear_timeout ) this->clear_timeout_task();
            } else if ( ret == 0 ) {
                // 超时，并没有事件发生
                printf("[ERROR] RPC_Proactor::run, poller wait timeout %d ms\n", timeout);
                this->clear_timeout_task();
            } else {
                // poller wait出现错误
                printf("[ERROR] RPC_Proactor::run, poller wait error\n");
            }
            return ret;
        }
        
    private:
        int on_acceptable(RPC_SocketListener * plistener) 
        {
            RPC_SocketChannel * p_channel = plistener->accept();
            if ( p_channel == nullptr ) {
                this->m_accept_handler(plistener, p_channel, RPC_Constants::Fail);
                printf("[ERROR] RPC_Proactor::on_acceptable, listener accept error\n");
                return RPC_Constants::Fail;
            }
            
            printf("[TRACE] RPC_Proactor::on_acceptable, listener %p, channel %p\n", plistener, p_channel);
            int ret = this->m_accept_handler(plistener, p_channel, RPC_Constants::Ok);
            if ( ret == RPC_Constants::Fail ) {
                printf("[ERROR] RPC_Proactor::on_acceptable, call back return fail\n");
                delete p_channel;
            }
            return ret;
        }
        
        int on_accept_timeout(RPC_SocketListener * plistener) {
            printf("[ERROR] RPC_Proactor::on_accept_timeout, poller wait error\n");
            return RPC_Constants::Ok;
        }
        
        int on_readable(RPC_SocketChannel *pchannel) {
            printf("[ERROR] RPC_Proactor::on_readable, poller wait error\n");
            return RPC_Constants::Ok;
        }
        
        int on_writable(RPC_SocketChannel *pch, RPC_TaskTimeoutQueue::Task *p_task);
        
        int on_connected(RPC_SocketChannel *p_ch) {
            printf("[TRACE] RPC_Proactor::on_connected\n");
            return this->m_connect_handler(p_ch, RPC_Constants::Ok);
        }
        
        void clear_timeout_task() {
            printf("[ERROR] RPC_Proactor::clear_timeout_task, poller wait error\n");
        }
        
        void process_events() {
            // 有事件发生
            typename Poller::Iterator iter = m_poller.events();
            while ( iter.has_next() ) {
                typename Poller::Event e = iter.next();
                RPC_TaskTimeoutQueue::TaskOwner * p_owner = (RPC_TaskTimeoutQueue::TaskOwner*)e.data();
                RPC_SocketObject *p_sock = p_owner->get_socket();
                    
                if ( e.events() & Poller::Event_Read ) {
                    printf("[TRACE] RPC_Proactor::process_events, get read event\n");
                    if ( p_sock->type() == RPC_SocketObject::Type_Listener ) {
                        int ret = this->on_acceptable((RPC_SocketListener*)p_sock);
                        if ( ret == RPC_Constants::Ok ) {
                            // 接受新连接完成
                            printf("[INFO] RPC_Proactor::process_events, listener get Finish\n");
                        } else if ( ret == RPC_Constants::Fail ) { 
                            printf("[ERROR] RPC_Proactor::process_events, listener get Fail\n" );
                        } else {
                            throw std::runtime_error("RPC_Proactor::run, Listener unknown callback returned value");
                        }
                    } else if ( p_sock->type() == RPC_SocketObject::Type_Channel ) {
                        int ret = this->on_readable((RPC_SocketChannel*)p_sock);
                        if ( ret == RPC_Constants::Ok ) {
                            // TODO 接下去怎么做
                            printf("[ERROR] RPC_Proactor::process_events, channel get Finish\n" );
                        } else {
                            throw std::runtime_error("RPC_Proactor::run, Channel unknown callback returned value");
                        }
                    } else {
                        char buf[128];
                        snprintf(buf, 128, "RPC_Proactor::run, bad socket type %d", p_sock->type());
                        throw std::runtime_error(buf);
                    }
                }
                if ( e.events() & Poller::Event_Write ) {
                    printf("[TRACE] RPC_Proactor::process_events, get write event\n");
                    if ( p_sock->type() == RPC_SocketObject::Type_Channel ) {
                        RPC_SocketChannel *p_channel = (RPC_SocketChannel*)p_sock;
                        auto p_task = p_owner->get_front_task(RPC_Constants::Write);  // 获取队列中一个写任务
                        if ( p_task ) { // 任务存在
                            if ( p_channel->state() == RPC_Constants::State_Connected ) {
                                int ret = this->on_writable(p_channel, p_task);
                                if ( ret == RPC_Constants::Ok ) {
                                    // TODO 接下去怎么做
                                    printf("[WARN] RPC_Proactor::process_events, channel write finish\n" );
                                    p_owner->pop_front_task(RPC_Constants::Write); // 任务完成，删除
                                } else {
                                    throw std::runtime_error("RPC_Proactor::run, Channel on writable returns unknown");
                                }
                            } else if ( p_channel->state() == RPC_Constants::State_Connecting) {
                                p_channel->state(RPC_Constants::State_Connected); // 修改状态为已连接
                                int ret = this->on_connected(p_channel);
                                if ( ret == RPC_Constants::Ok ) {
                                    // TODO 接下去怎么做
                                    p_owner->pop_front_task(RPC_Constants::Write); // 任务完成，删除
                                    printf("[WARN] RPC_Proactor::process_events, channel connected\n" );
                                } else {
                                    throw std::runtime_error("RPC_Proactor::run, Channel on connected returns unknwon");
                                }
                            }
                        } else {
                            throw std::runtime_error("RPC_Proactor::run, no write task");
                        }
                    } else {
                        printf("[ERROR] RPC_Proactor::process_events, unknown write socket object\n");
                        throw std::runtime_error("RPC_Proactor::process_events, unknown write socket object");
                    }
                }
            } // end while
            printf("[TRACE] RPC_Proactor::process_events\n" );
        } // end of process_events
        
    }; // class RPC_Proactor
    
    template<class Poller>
    inline 
    int RPC_Proactor<Poller>::on_writable(
        RPC_SocketChannel *pch, RPC_TaskTimeoutQueue::Task *p_task) 
    {
        printf("[TRACE] RPC_Proactor<Poller>::on_writable\n");
        RPC_Message &r_msg = p_task->message();
        RPC_Message::Buffer_Sequence &r_bufseq = r_msg.buffers();
        Mutable_Buffer_Sequence::Iterator it = r_bufseq.latest();
        if ( it == r_bufseq.end() ) {
            throw std::runtime_error("RPC_Proactor<Poller>::on_writable, no buffer");
        }
        size_t total_size = 0;
        m_send_iovec.resize(0);
        printf("[TRACE] RPC_Proactor<Poller>::on_writable, size %ld, cap %ld\n",
            m_send_iovec.size(), m_send_iovec.capacity());
        for(; it != r_bufseq.end(); ++it ) {
            size_t s = it->size() - it->position();
            if ( s == 0 ) continue;
            
            total_size += s;
            if ( m_send_iovec.capacity() == m_send_iovec.size() ) {
                printf("[TRACE] RPC_Proactor<Poller>::on_writable, reserve: size %ld, cap %ld\n",
                    m_send_iovec.size(), m_send_iovec.capacity());
                m_send_iovec.reserve(m_send_iovec.capacity() * 2);
            }
            m_send_iovec.push_back(iovec{it->ptr<>(it->position()), s});
        } // end for
        
        ssize_t ret = pch->get_socket().send(m_send_iovec);
        if ( ret > 0 ) {
            printf("[TRACE] RPC_Proactor::on_writable, %ld bytes sent\n", ret);
            if ( ret >= total_size ) {
                printf("[TRACE] RPC_Proactor::on_writable send ok not impl\n");
                int r = this->m_send_handler(pch, r_msg, RPC_Constants::Ok);
                if ( r == RPC_Constants::Ok ) {
                    printf("[ERROR] RPC_Proactor::on_writable send ok handler return not impl\n");
                }
                // todo: remove from proactor
                printf("[ERROR] RPC_Proactor::on_writable remove from proactor not impl\n");
            } else {
                printf("[ERROR] RPC_Proactor::on_writable send part not impl\n");
            }
        } else if ( ret == 0 ) {
            printf("[ERROR] RPC_Proactor::on_writable, no byte sent\n");
        } else { // ret < 0
            if ( errno == EAGAIN ) {
                printf("[ERROR] RPC_Proactor::on_writable, EAGAIN\n");
            } else {
                printf("[ERROR] RPC_Proactor::on_writable,%d, %s %ld\n", errno, strerror(errno));
            } 
        }
        
        printf("[TRACE] RPC_Proactor::on_writable, total size %ld\n", total_size);
        return RPC_Constants::Ok;
    }
    
    template<class Poller>
    inline 
    bool RPC_Proactor<Poller>::add_read(RPC_SocketObject *sockobj, RPC_Message& msg, int64_t expire)
    {
        RPC_TaskTimeoutQueue::TaskOwner * p_owner = m_task_timeout_queue.find_owner(sockobj);
        assert(p_owner);
        
        bool isok = m_task_timeout_queue.push_task(p_owner, RPC_Constants::Read, msg, expire); 
        assert( isok );
        
        printf("[TRACE] RPC_Proactor::add_read(sockobj), expire %lld\n", expire);
        
        int events = 0;
        if ( p_owner->get_front_task(RPC_Constants::Read ) != nullptr ) events |= Poller::Event_Read;
        if ( p_owner->get_front_task(RPC_Constants::Write ) != nullptr ) events |= Poller::Event_Write;
        
        printf("[TRACE] RPC_Proactor::add_read, poller set, event %d\n", events);
        isok = m_poller.set(sockobj->get_socket().handle(), events, p_owner);
        if ( !isok ) {
            printf("[ERROR] RPC_Proactor::add_read(sockobj) error\n");
            return false;
        }
        
        // m_task_timeout_queue.push(timeout);  // 放入超时队列
        return isok;
    }
    
    template<class Poller>
    inline 
    bool RPC_Proactor<Poller>::add_write(RPC_SocketObject *sockobj, RPC_Message &msg, int64_t expire)
    {
        RPC_TaskTimeoutQueue::TaskOwner * p_owner = m_task_timeout_queue.find_owner(sockobj);
        assert(p_owner);
            
        bool isok = m_task_timeout_queue.push_task(p_owner, RPC_Constants::Write, msg, expire); 
        assert( isok );
            
        printf("[TRACE] RPC_Proactor::add_write(sockobj), expire %lld\n", expire);
            
        int events = 0;
        if ( p_owner->get_front_task(RPC_Constants::Read ) != nullptr ) events |= Poller::Event_Read;
        if ( p_owner->get_front_task(RPC_Constants::Write ) != nullptr ) events |= Poller::Event_Write;
            
        printf("[TRACE] RPC_Proactor::add_write, poller set, event %d\n", events);
        isok = m_poller.set(sockobj->get_socket().handle(), events, p_owner);
        if ( !isok ) {
            printf("[ERROR] RPC_Proactor::add_write(sockobj) error\n");
            return false;
        }
        
        return isok;
    } // end if RPC_Proactor::add_write
    
} // end of namespace rpc 
} // end of namespace everest 


#endif // INCLUDE_EVEREST_RPC_RPC_PROACTOR_H