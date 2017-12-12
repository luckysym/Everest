#ifndef INCLUDE_EVEREST_RPC_RPC_SERVER_H
#define INCLUDE_EVEREST_RPC_RPC_SERVER_H

#include <everest/date_time.h>
#include <everest/net/sock_addr.h>
#include <everest/net/socket.h>
#include <everest/net/epoller.h>
#include <everest/rpc/RPC_Socket.h>

#include <assert.h>
#include <sys/epoll.h>

#include <memory>
#include <unordered_set>
#include <sstream>
#include <functional>
#include <stdexcept>
#include <queue>
#include <map>
#include <unordered_map>

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
        
        public:
            Task(TaskOwner *owner, int type, int64_t exp) 
                : m_owner_ref(owner), m_type(type), m_expire_time(exp) {}
        
            int64_t expire_time() const {  return m_expire_time; }
            
            int type() const { return m_type; }
            
            TaskOwner * owner() { return m_owner_ref; }
        };
        
        class TaskOwner 
        {
        private:
            RPC_SocketObject * m_owner_ref;
            std::queue<Task> m_wr_queue;  // 读任务队列
            std::queue<Task> m_rd_queue;  // 写任务队列
            
        public:
            TaskOwner(RPC_SocketObject* p) : m_owner_ref(p) {}
        
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
        
        Task * push_task(TaskOwner * owner, int type, int64_t expire) 
        {
            // 先写入owner任务队列
            Task * p_task = owner->push_task(Task(owner, type, expire));
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
    
    template<class Poller = net::EPoller, class TaskTimeoutQueue = RPC_TaskTimeoutQueue>
    class RPC_Proactor 
    {
    private:
        Poller           m_poller;
        TaskTimeoutQueue m_task_timeout_queue;   // 任务超时队列
        
    public:
        bool reg(RPC_SocketObject *sockobj) 
        {
            // 任务超时队列中注册socket对象
            typename TaskTimeoutQueue::TaskOwner * p_owner = m_task_timeout_queue.add_owner(sockobj);
            assert(p_owner);
            
            bool isok = m_poller.add(sockobj->get_socket().handle(), 0, p_owner);
            if ( !isok ) {
                printf("[ERROR] RPC_Proactor::reg(sockobj) error\n");
                return false;
            }
            return true;
        }

        bool add_read(RPC_SocketObject *sockobj, int64_t expire)
        {
            typename TaskTimeoutQueue::TaskOwner * p_owner = m_task_timeout_queue.find_owner(sockobj);
            assert(p_owner);
            
            bool isok = m_task_timeout_queue.push_task(p_owner, RPC_Constants::Read, expire); 
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

        int run() {
            int64_t now = DateTime::get_timestamp();
            if ( m_task_timeout_queue.empty() ) {
                printf("[INFO] RPC_Proactor::run, no task \n");
                return 0;
            }
            
            bool  need_clear_timeout = false;   // 是否在wait后需要清理超时任务
            
            typename TaskTimeoutQueue::Task * p_task = m_task_timeout_queue.get_front_timeout();
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
        int on_acceptable(RPC_SocketListener * plistener) {
            printf("[ERROR] RPC_Proactor::on_readable, poller wait error\n");
            return RPC_Constants::Finish;
        }
        
        int on_accept_timeout(RPC_SocketListener * plistener) {
            printf("[ERROR] RPC_Proactor::on_accept_timeout, poller wait error\n");
            return RPC_Constants::Finish;
        }
        
        void clear_timeout_task() {
            printf("[ERROR] RPC_Proactor::clear_timeout_task, poller wait error\n");
        }
        
        void process_events() {
            // 有事件发生
            typename Poller::Iterator iter = m_poller.events();
            while ( iter.has_next() ) {
                typename Poller::Event e = iter.next();
                if ( e.events() & Poller::Event_Read ) {
                    RPC_SocketObject * p_sock = (RPC_SocketObject*)e.data();
                    if ( p_sock->type() == RPC_SocketObject::Type_Listener ) {
                        int ret = this->on_acceptable((RPC_TcpSocketListener*)p_sock);
                        if ( ret == RPC_Constants::Finish ) {
                            // TODO 接下去怎么做
                        } else {
                            throw std::runtime_error("RPC_Proactor::run, unknown callback returned value");
                        }
                    } else {
                        throw std::runtime_error("RPC_Proactor::run, bad socket type");
                    }
                } // end if
            } // end while
        } // end of process_events
    }; // class RPC_Proactor
    
    class RPC_Message {};
    
    class RPC_TcpSocketService_Impl
    {
    public:
        typedef RPC_TcpSocketChannel   ChannelType;
        typedef RPC_TcpSocketListener  ListenerType;
        typedef RPC_Message            MessageType;
        typedef RPC_Proactor<>         ProactorType;
    }; // end of class RPC_TcpSocketService_Impl
    
    template<class Impl = RPC_TcpSocketService_Impl>
    class RPC_Service
    {
    public:
    
        typedef typename Impl::ChannelType    ChannelType;
        typedef typename Impl::ListenerType   ListenerType;
        typedef typename Impl::ChannelType*   ChannelPtr;
        typedef typename Impl::ListenerType*  ListenerPtr;
        typedef typename Impl::MessageType    MessageType;
        typedef typename Impl::ProactorType   ProactorType;
        
        struct AsyncTask
        {
            int          task_type;
            ChannelPtr   p_channel;
            ListenerPtr  p_listener;
            MessageType  message;
            int64_t      expire_time;
        };  // end struct AsyncTask 
        typedef std::queue<AsyncTask>         AsyncTaskQueue;
        
        static const int Task_AsyncAccept  = 1;
        static const int Task_AsyncWrite   = 2;
        
    private:
        AsyncTaskQueue m_async_taks_queue;
        ProactorType   m_proactor;
        
        std::function<void (ListenerPtr, int)> m_accept_error_handler;
         
    private:
        RPC_Service(const RPC_Service&) = delete;
        RPC_Service& operator=(const RPC_Service&) = delete;

    public: 
        RPC_Service();
        ~RPC_Service();
        
        template<class ConnHandler>
        void        set_conn_handle(ConnHandler handler);
        
        template<class RecvHandler>
        void        set_recv_handle(RecvHandler handler);
        
        template<class SendHandler>
        void        set_send_handle(SendHandler handler);
        
        bool        open_channel(const char * endpoint, int timeout);
        bool        close_channel(ChannelPtr channel);
        
        ListenerPtr open_listener(const char * endpoint);
        bool        close_listener(ListenerPtr listener);
        
        bool        post_accept(ListenerPtr listener, int timeout);
        bool        post_receive(ChannelPtr channel, MessageType & rMessage, int timeout);
        bool        post_send(ChannelPtr channel, MessageType & rMessage, int timeout);
        
        int         run();
    }; // end of class RPC_Service 
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest {
namespace rpc {
    
    template<class Impl>
    RPC_Service<Impl>::RPC_Service() {}
    
    template<class Impl>
    RPC_Service<Impl>::~RPC_Service() {}
    
    template<class Impl>
    typename RPC_Service<Impl>::ListenerPtr 
    RPC_Service<Impl>::open_listener(const char * endpoint)
    {
        // 打开监听器
        ListenerPtr ptrListener(new ListenerType());
        bool isok = ptrListener->open(endpoint);
        if ( !isok ) {
            printf("[ERROR] RPC_Service::open_listener, open listener failed\n");
            return ListenerPtr(nullptr);
        }
        
        // 注册到Reactor
        isok = m_proactor.reg(ptrListener);
        if ( !isok ) {
            printf("[ERROR] RPC_Service::open_listener, failed reg\n");
            delete ptrListener;
            return ListenerPtr(nullptr);
        }
        
        // 返回监听器指针
        return ptrListener;
    } // end of RPC_Service<Impl>::open_listener
    
    template<class Impl>
    bool RPC_Service<Impl>::post_accept(ListenerPtr listener, int timeout)
    {
        int64_t now = DateTime::get_timestamp();
        AsyncTask task;
        
        task.task_type   = Task_AsyncAccept;
        task.p_listener  = listener;
        if ( timeout < 0 ) task.expire_time = RPC_Constants::Max_Expire_Time;
        else task.expire_time = now + timeout * 1000;
        
        // todo: make lock
        m_async_taks_queue.push(task);
        
        return true;
    }
    
    template<class Impl>
    bool RPC_Service<Impl>::open_channel(const char * endpoint, int timeout) 
    {
        int64_t now = DateTime::get_timestamp();
        ChannelPtr p_channel = new ChannelType;
        
        bool isok = p_channel->open(endpoint);
        if ( !isok ) {
            printf("[ERROR] RPC_Service<Impl>::open_channel, failed to open channel, %d, %s\n", timeout, endpoint);
            return false;
        }

        AsyncTask task;
        task.task_type  = Task_AsyncWrite;
        task.p_channel  = p_channel;
        if ( timeout < 0 ) task.expire_time = RPC_Constants::Max_Expire_Time;
        else task.expire_time = now + timeout * 1000;
        
        // todo: make lock
        m_async_taks_queue.push(task);
        
        return true;
    }
    
    template<class Impl>
    int RPC_Service<Impl>::run()
    {
        bool isok;
        while ( !m_async_taks_queue.empty() ) {
            AsyncTask & task = m_async_taks_queue.front();
            
            if ( task.task_type == Task_AsyncAccept) {
                printf("[TRACE] RPC_Service::run, new accept task\n");
                m_proactor.add_read(task.p_listener, task.expire_time);
            } else {
                printf("[ERROR] RPC_Service::run, unknown task type %d\n", task.task_type);
            }
            m_async_taks_queue.pop();
        }
        
        int ret = m_proactor.run();
        if ( ret < 0 ) {
            printf("[ERROR] RPC_Service::run, reactor run failed\n");
        }
        return ret;
    }
    
} // end of namespace rpc 
} // end of namespace everest

#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
