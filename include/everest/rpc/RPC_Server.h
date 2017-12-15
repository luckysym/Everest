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
    public:
        static const size_t Header_Length = 24;
        
        // Message Type
        static const char   Type_Request  = 0;
        
        // Message field index
        static const size_t Idx_Length = 8;
        
        // byte endian
        static const char   Little_Endian = 0;
        static const char   Big_Endian = 1; 
        
    private:
        typedef std::vector<Mutable_Byte_Buffer> Buffer_Sequence;
        Buffer_Sequence m_buffer_seq;
        
    public:
        RPC_Message() {}
    
        RPC_Message(Mutable_Byte_Buffer &buf) 
        {
            m_buffer_seq.reserve(16);
            m_buffer_seq.push_back(buf);
        }
        
        ~RPC_Message() {}
        
        bool init_header();
        bool update_header();
        bool add_buffer(Mutable_Byte_Buffer & buf);
        
    }; // end of class RPC_Message 
    
    bool RPC_Message::update_header()
    {
        uint32_t total_len = 0;
        Buffer_Sequence::iterator it = m_buffer_seq.begin();
        for(; it != m_buffer_seq.end(); ++it) {
            total_len += it->size();
        }
        m_buffer_seq[0].data<uint32_t>(Idx_Length) = total_len;;
    }
    
    bool RPC_Message::add_buffer(Mutable_Byte_Buffer & buf) {
        if ( m_buffer_seq.size() == m_buffer_seq.capacity() ) {
            m_buffer_seq.reserve(m_buffer_seq.capacity() * 2);
        }
        m_buffer_seq.push_back(buf);
    }
    
    bool RPC_Message::init_header() {
        Buffer_Sequence::iterator it = m_buffer_seq.begin();
        if ( it == m_buffer_seq.end() ) {
            printf("[ERROR] RPC_Message::init_header, no buffer\n");
            return false;
        }
        Mutable_Byte_Buffer &buf = *it;
        bool isok = buf.resize(Header_Length);
        if ( !isok ) {
            printf("[ERROR] RPC_Message::init_header, no enough space\n");
            return false;
        }
        
        // magic
        buf[0] = 'G'; buf[1] = 'I'; buf[2] = 'O'; buf[3] = 'P';
        // version
        buf[4] = '0'; buf[5] = '0';  
        // flags/byte order and message type 
        buf[6] = Little_Endian; buf[7] = Type_Request;
        // length
        buf.data<uint32_t>(8) = 0;
        
        return true;
    } // end of RPC_Message::init_header
    
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
        
        // channel connected handler 
        class ConnectHandler
        {
            std::function<int (ChannelPtr, int)> &m_rhandler;
        public:
            ConnectHandler(std::function<int (ChannelPtr, int)> &handler)
                : m_rhandler(handler) {}
                
            int operator()(ChannelPtr p_channel, int ec) {
                printf("[TRACE] RPC_Service::ConnectHandler()\n");
                return this->m_rhandler(p_channel, ec);
            }
        };
        
    private:
        AsyncTaskQueue m_async_task_queue;
        ProactorType   m_proactor;
        
        std::function<int (ListenerPtr, ChannelPtr, int)> m_accept_handler;
        std::function<int (ChannelPtr, int)> m_connect_handler;
        
    private:
        RPC_Service(const RPC_Service&) = delete;
        RPC_Service& operator=(const RPC_Service&) = delete;

    public: 
        RPC_Service();
        ~RPC_Service();
        
        template<class ConnHandler>
        void        set_conn_handler(const ConnHandler &handler) { m_connect_handler = handler; }
        
        template<class RecvHandler>
        void        set_recv_handler(const RecvHandler &handler);
        
        template<class SendHandler>
        void        set_send_handler(const SendHandler &handler);
        
        template<class AcceptHandler>
        void        set_accept_handler(const AcceptHandler &handler) { m_accept_handler = handler; }
        
        bool        open_channel(const char * endpoint, int timeout);
        bool        close_channel(ChannelPtr channel);
        
        ListenerPtr open_listener(const char * endpoint);
        bool        close_listener(ListenerPtr listener);
        
        bool        add_channel(ChannelPtr channel);
        
        bool        post_accept(ListenerPtr listener, int timeout);
        bool        post_receive(ChannelPtr channel, MessageType cMessage, int timeout);
        bool        post_send(ChannelPtr channel, MessageType  cMessage, int timeout);
        
        int         run_once();
    }; // end of class RPC_Service 
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest {
namespace rpc {
    
    template<class Impl>
    RPC_Service<Impl>::RPC_Service() {
        m_proactor.set_accept_handler(AcceptHandler(m_accept_handler));
        m_proactor.set_connect_handler(ConnectHandler(m_connect_handler));
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
    bool RPC_Service<Impl>::post_send(ChannelPtr channel, MessageType msg, int timeout) 
    {
        int64_t exp = RPC_Constants::Max_Expire_Time;
        if ( timeout > 0 ) {
            int64_t now = DateTime::get_timestamp();
            exp = now + timeout * 1000;
        }
        
        AsyncTask task(Task_Async_Write, channel, msg, exp);
        m_async_task_queue.push(task);
        printf("[TRACE] RPC_Service<Impl>::post_send, %d\n", channel->get_socket().handle());
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
