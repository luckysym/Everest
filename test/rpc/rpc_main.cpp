#include <pthread.h>
#include <everest/rpc/RPC_Server.h>
#include <iostream>
#include <unistd.h>

namespace rpc = everest::rpc;

#define CHECK( x ) \
    do {\
        if ( !(x) ) return -1; \
    } while (0) 


int start_server(int argc, char **argv);
int start_client(int argc, char **argv);
int stop_server();
int stop_client();

void * run_server(void *);
void * run_client(void *);

int server_result = 0;
int client_result = 0;

pthread_t server_tid = 0;
pthread_t client_tid = 0;

#define RPC_LOCAL_ENDPOINT   "127.0.0.1:9999"

int main(int argc, char **argv)
{
    using namespace std;
    
    int ret;
    ret = start_server(argc, argv);
    if ( ret != 0 ) {
        cout<<"[error] start server fail"<<endl;
        return ret;
    } 
    sleep(1);
    ret = start_client(argc, argv);
    if ( ret != 0 ) {
        cout<<"[error] start client fail"<<endl;
        return ret;
    } 
    
    ret = stop_client();
    if ( ret != 0 ) {
        cout<<"[error] stop client fail"<<endl;
        return ret;
    } 
    ret = stop_server();
    if ( ret != 0 ) {
        cout<<"[error] stop client fail"<<endl;
        return ret;
    } 
    return ret;
}

int start_server(int argc, char **argv)
{
    using namespace std;
    int ret = pthread_create(&server_tid, nullptr, run_server, nullptr);
    if ( ret < 0 ) {
        cout<<"[error] create server thread fail"<<endl;
        return ret;
    }
    return server_result;
}

int stop_server()
{
    pthread_join(server_tid, nullptr);
    return 0;
}

int start_client(int argc, char **argv)
{
    using namespace std;
    int ret = pthread_create(&client_tid, nullptr, run_client, nullptr);
    if ( ret < 0 ) {
        cout<<"[error] create client thread fail"<<endl;
        return ret;
    }
    return client_result;
}

int stop_client()
{
    pthread_join(client_tid, nullptr);
    return 0;
}

class ServerAcceptHandler
{ 
private:
    rpc::RPC_Service<> &m_service;
    
public:
    ServerAcceptHandler(rpc::RPC_Service<> &service)
        : m_service(service) {}

    int operator()(rpc::RPC_SocketListener *p_listener, rpc::RPC_SocketChannel * p_channel, int ec)
    {
        printf("[TRACE] Test ServerAcceptHandler, %p, %p, %d\n", p_listener, p_channel, ec);
        if ( ec == rpc::RPC_Constants::Ok ) {
            printf("[TRACE] Test ServerAcceptHandler ok, %p, %p, %d\n", p_listener, p_channel, ec);
            bool isok = m_service.add_channel(p_channel);
            if ( !isok ) {
                printf("[ERROR] Test ServerAcceptHandler add channel error, %p, %p, %d\n", p_listener, p_channel, ec);
                return rpc::RPC_Constants::Fail;
            }
            
            everest::Mutable_Byte_Buffer buf(new char[1024], 1024);
            buf.limit(24);
            everest::Mutable_Buffer_Sequence * seq = new everest::Mutable_Buffer_Sequence();
            seq->push_back(buf);
            isok = m_service.post_receive(p_channel, rpc::RPC_Message(*seq), 5000);
            if ( isok ) {
                return rpc::RPC_Constants::Ok;
            } else {
                printf("[ERROR] Test ServerAcceptHandler channel async read failed, %p, %p, %d\n", p_listener, p_channel, ec);
                return rpc::RPC_Constants::Fail;
            }
        } else {
            printf("[ERROR] Test ServerAcceptHandler fail, %p, %p, %d\n", p_listener, p_channel, ec);
            return rpc::RPC_Constants::Fail;
        }
    }
};

class ServerRecvHandler 
{
    rpc::RPC_Service<> &m_service;
public:
    ServerRecvHandler(rpc::RPC_Service<> &service) : m_service(service) {}
    
    int operator()(rpc::RPC_SocketChannel *p_channel, rpc::RPC_Message &msg, int ec) {
        auto & buffers = msg.buffers();
        size_t buf_size = buffers.size();
        size_t msg_size = msg.size();
        
        if ( buf_size < msg_size ) {
            printf("[TRACE] Test ServerRecvHandler, part message,  msg %lu, buf %lu\n", msg_size, buf_size);
            auto iter = buffers.latest();
            if ( iter->capacity() >= msg_size ) {
                iter->limit(msg_size);  // 在当前缓存上扩展
            } else {
                // 缓存不足，需要增加一个缓存
                everest::Mutable_Byte_Buffer buf(new char[msg_size - buf_size], msg_size - buf_size);
                buffers.push_back(buf);
            }
            printf("[TRACE] Test ServerRecvHandler, new buffer add, continue\n");
            return rpc::RPC_Constants::Continue;
        } else {
            printf("[TRACE] Test ServerRecvHandler, full message,  msg %lu, buf %lu\n", msg_size, buf_size);
            // 清理消息中的缓存
            auto iter = buffers.begin();
            for ( ; iter != buffers.end(); ++iter ) {
                char * p = iter->ptr();
                iter->detach();
                delete[] p;
            }
            buffers.clear();
            auto p_buffers = msg.detach_buffers();
            
            // 生成回复
            p_buffers->push_back(everest::Mutable_Byte_Buffer(new char[64], 64));
            auto & buf = p_buffers->front();
            rpc::RPC_Message newmsg(*p_buffers);
            newmsg.init_header();
            strcpy(buf.ptr(rpc::RPC_Message::Header_Length), "hello response");
            buf.size(buf.size() + strlen("hello response"));
            newmsg.update_header();
            
            // 发送回复
            bool send_ok = m_service.post_send(p_channel, newmsg, 5000);
            if ( !send_ok ) {
                printf("[ERROR] Test ServerRecvHandler, send fail\n");
                return rpc::RPC_Constants::Fail;
            } 
            printf("[INFO] Test ServerRecvHandler, send ok\n");
            return rpc::RPC_Constants::Ok;
        }
        return rpc::RPC_Constants::Fail;
    }
};

void * run_server(void *)
{
    rpc::RPC_Service<> server;
    
    server.set_accept_handler(ServerAcceptHandler(server));
    server.set_recv_handler(ServerRecvHandler(server));
    
    rpc::RPC_Service<>::ListenerPtr ptrListener= server.open_listener(RPC_LOCAL_ENDPOINT);
    if ( !ptrListener ) { throw std::runtime_error("open listener failed"); } 
    
    bool isok = server.post_accept(ptrListener, 5000);
    if ( !isok ) { throw std::runtime_error("post accept failed");}
    
    for(int i = 0; i < 2; ++i) {
        int ret = server.run_once();
        printf("Server Run Once %d\n", ret);
        if ( ret < 0 ) break;
    }
    
    printf("Server Exit\n");
    return nullptr;
}

class ClientConnectHandler
{
private:
    rpc::RPC_Service<> &m_service;
    
public:
    ClientConnectHandler(rpc::RPC_Service<> &service)
        : m_service(service) {}
    
    int operator()(rpc::RPC_SocketChannel *p_channel, int ec) {
        printf("[TRACE] Test ClientConnectHandler %d\n", ec);
    
        everest::Mutable_Byte_Buffer buf(new char[32], 32);
        everest::Mutable_Byte_Buffer buf2(new char[128], 128);
        int len = snprintf(buf2.ptr(), buf2.capacity(), "hello request");
        buf2.size(len);
        everest::Mutable_Buffer_Sequence * bufseq = new everest::Mutable_Buffer_Sequence();
        bufseq->push_back(buf);
        bufseq->push_back(buf2);
        rpc::RPC_Message msg(*bufseq);
        msg.init_header();
        msg.update_header();
        
        bool isok = m_service.post_send(p_channel, msg, 2000);
        if ( !isok ) {
            printf("[TRACE] Test ClientConnectHandler post send fail\n");
            return rpc::RPC_Constants::Fail;
        }
        return rpc::RPC_Constants::Ok;
    }
};

class ClientSendHandler
{
    rpc::RPC_Service<> &m_service;
public:
    ClientSendHandler(rpc::RPC_Service<> &service)
        : m_service(service) {}

    int operator()(rpc::RPC_SocketChannel *p_channel, rpc::RPC_Message &msg, int ec) {
        printf("[ERROR] Test ClientSentHandler %d\n", ec);
        return rpc::RPC_Constants::Ok;
    }
};

void * run_client(void *)
{
    rpc::RPC_Service<> client;
    client.set_conn_handler(ClientConnectHandler(client));
    client.set_send_handler(ClientSendHandler(client));
    
    bool isok = client.open_channel(RPC_LOCAL_ENDPOINT, 3000);
    
    for(int i = 0; i < 2; ++i) {
        int ret = client.run_once();
        printf("Client Run Once %d\n", ret);
        if ( ret < 0 ) break;
    }
    printf("Client Exit\n");
    return nullptr;
}