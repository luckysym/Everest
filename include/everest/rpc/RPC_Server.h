#ifndef INCLUDE_EVEREST_RPC_RPC_SERVER_H
#define INCLUDE_EVEREST_RPC_RPC_SERVER_H

#include <everest/date_time.h>
#include <everest/buffer.h>
#include <everest/net/sock_addr.h>
#include <everest/net/socket.h>
#include <everest/net/epoller.h>
#include <everest/rpc/RPC_Socket.h>
#include <everest/rpc/RPC_Proactor.h>

#include <memory>
#include <unordered_set>
#include <sstream>
#include <stdexcept>

namespace everest
{
namespace rpc 
{
    class RPC_Message 
    {
    private:
        Mutable_Byte_Buffer *m_buffer;
        
    public:
        RPC_Message() : m_buffer(nullptr) {}
    
        RPC_Message(Mutable_Byte_Buffer * p_buf) 
            : m_buffer(p_buf) 
        {}
        
        ~RPC_Message() { m_buffer = nullptr; } 
        
        Mutable_Byte_Buffer * buffer() { return m_buffer; }
        const Mutable_Byte_Buffer * buffer() const { return m_buffer; }
    }; // end of class RPC_Message 
    
    class RPC_TcpSocketService_Impl
    {
    public:
        typedef RPC_SocketChannel      ChannelType;
        typedef RPC_SocketListener     ListenerType;
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

        static const int Task_Async_Accept  = 1;
        static const int Task_Async_Write   = 2;
        static const int Task_Async_Read    = 3;
        static const int Task_Async_Add     = 4;   // add channel or listener to proactor service
        
        struct AsyncTask
        {
            int          task_type;
            ChannelPtr   p_channel;
            ListenerPtr  p_listener;
            MessageType  message;
            int64_t      expire_time;
            
            AsyncTask() 
                : task_type(0), p_channel(nullptr), p_listener(nullptr), expire_time(RPC_Constants::Max_Expire_Time)
            {}
            
            AsyncTask(int type, ChannelPtr ch) 
                : task_type( type ) , p_channel(ch), p_listener(nullptr), expire_time(RPC_Constants::Max_Expire_Time)
            {}
                
            AsyncTask(int type, ChannelPtr ch, const MessageType &msg, int64_t exp)
                : task_type(type), p_channel(ch), p_listener(nullptr), message(msg), expire_time(exp)
            {}

            AsyncTask(int type, ListenerPtr listener, const MessageType &msg, int64_t exp)
                : task_type(type), p_channel(nullptr), p_listener(listener), message(msg), expire_time(exp)
            {}

        };  // end struct AsyncTask 
        typedef std::queue<AsyncTask>         AsyncTaskQueue;
        

        
        class AcceptHandler
        {
            std::function<int (ListenerPtr, ChannelPtr, int)> &m_rhandler;
        public:
        
            AcceptHandler(std::function<int (ListenerPtr, ChannelPtr, int)> & handler)
                : m_rhandler(handler) 
            {}
            
            int operator()(ListenerPtr p_listener, ChannelPtr p_channel, int ec) 
            {
                printf("[TRACE] RPC_Service::AcceptHandler()\n");
                return m_rhandler(p_listener, p_channel, ec);
            }
        };
        
    private:
        AsyncTaskQueue m_async_task_queue;
        ProactorType   m_proactor;
        
        std::function<int (ListenerPtr, ChannelPtr, int)> m_accept_handler;
        
    private:
        RPC_Service(const RPC_Service&) = delete;
        RPC_Service& operator=(const RPC_Service&) = delete;

    public: 
        RPC_Service();
        ~RPC_Service();
        
        template<class ConnHandler>
        void        set_conn_handle(const ConnHandler &handler);
        
        template<class RecvHandler>
        void        set_recv_handle(const RecvHandler &handler);
        
        template<class SendHandler>
        void        set_send_handle(const SendHandler &handler);
        
        template<class AcceptHandler>
        void        set_accept_handler(const AcceptHandler &handler) { m_accept_handler = handler; }
        
        bool        open_channel(const char * endpoint, int timeout);
        bool        close_channel(ChannelPtr channel);
        
        ListenerPtr open_listener(const char * endpoint);
        bool        close_listener(ListenerPtr listener);
        
        bool        add_channel(ChannelPtr channel);
        
        bool        post_accept(ListenerPtr listener, int timeout);
        bool        post_receive(ChannelPtr channel, MessageType cMessage, int timeout);
        bool        post_send(ChannelPtr channel, MessageType & rMessage, int timeout);
        
        int         run_once();
    }; // end of class RPC_Service 
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest {
namespace rpc {
    
    template<class Impl>
    RPC_Service<Impl>::RPC_Service() {
        m_proactor.set_accept_handler(AcceptHandler(m_accept_handler));
    }
    
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
        
        task.task_type   = Task_Async_Accept;
        task.p_listener  = listener;
        if ( timeout < 0 ) task.expire_time = RPC_Constants::Max_Expire_Time;
        else task.expire_time = now + timeout * 1000;
        
        // todo: make lock
        m_async_task_queue.push(task);
        
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

        // write任务
        AsyncTask task(Task_Async_Write, p_channel);
        if ( timeout < 0 ) task.expire_time = RPC_Constants::Max_Expire_Time;
        else task.expire_time = now + timeout * 1000;
        
        // todo: make lock
        m_async_task_queue.push(AsyncTask(Task_Async_Add, p_channel)); // add任务
        m_async_task_queue.push(task);  // write任务，仅用于检测
        printf("[TRACE] RPC_Service<Impl>::open_channel, %s, %d\n", endpoint, timeout);
        return true;
    }
    
    template<class Impl>
    bool RPC_Service<Impl>::add_channel(ChannelPtr channel) 
    {
        AsyncTask task(Task_Async_Add, channel);
        m_async_task_queue.push(task);
        printf("[TRACE] RPC_Service<Impl>::add_channel, %d\n", channel->get_socket().handle());
        return true;
    }
    
    template<class Impl>
    bool RPC_Service<Impl>::post_receive(ChannelPtr channel, MessageType msg, int timeout) 
    {
        int64_t exp = RPC_Constants::Max_Expire_Time;
        if ( timeout > 0 ) {
            int64_t now = DateTime::get_timestamp();
            exp = now + timeout * 1000;
        }
        
        AsyncTask task(Task_Async_Read, channel, msg, exp);
        m_async_task_queue.push(task);
        printf("[TRACE] RPC_Service<Impl>::post_receive, %d\n", channel->get_socket().handle());
        return true;
    }
    
    template<class Impl>
    int RPC_Service<Impl>::run_once()
    {
        bool isok;
        while ( !m_async_task_queue.empty() ) {
            AsyncTask & task = m_async_task_queue.front();
            
            if ( task.task_type == Task_Async_Accept) {
                printf("[TRACE] RPC_Service::run, new accept task\n");
                m_proactor.add_read(task.p_listener, task.expire_time);
            } else if (task.task_type == Task_Async_Read) {
                printf("[TRACE] RPC_Service::run, new read task\n");
                m_proactor.add_read(task.p_channel, task.expire_time);
            } else if (task.task_type == Task_Async_Write ) {
                printf("[TRACE] RPC_Service::run, new write task\n");
                m_proactor.add_write(task.p_channel, task.expire_time);
            } else if (task.task_type == Task_Async_Add)  {
                printf("[TRACE] RPC_Service::run, new add task %d\n", task.task_type);
                if ( task.p_channel != nullptr ) m_proactor.reg(task.p_channel);
                if ( task.p_listener != nullptr) m_proactor.reg(task.p_listener);
            } else {
                printf("[ERROR] RPC_Service::run, unknown task type %d\n", task.task_type);
            }
            m_async_task_queue.pop();
        }
        
        int ret = m_proactor.run_once();
        if ( ret < 0 ) {
            printf("[ERROR] RPC_Service::run, reactor run failed\n");
        }
        return ret;
    }

} // end of namespace rpc 
} // end of namespace everest

#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
