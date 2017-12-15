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
            isok = m_service.post_receive(p_channel, rpc::RPC_Message(buf), 5000);
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

void * run_server(void *)
{
    rpc::RPC_Service<> server;
    
    server.set_accept_handler(ServerAcceptHandler(server));
    
    rpc::RPC_Service<>::ListenerPtr ptrListener= server.open_listener(RPC_LOCAL_ENDPOINT);
    if ( !ptrListener ) { throw std::runtime_error("open listener failed"); } 
    
    bool isok = server.post_accept(ptrListener, 5000);
    if ( !isok ) { throw std::runtime_error("post accept failed");}
    
    int result = server.run_once();
    if ( result < 0 ) { throw std::runtime_error("server run failed");}
    
    printf("Server Exit, result %d\n", result);
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
        buf2.resize(len);
        rpc::RPC_Message msg(buf);
        msg.init_header();
        msg.add_buffer(buf2);
        msg.update_header();
        
        bool isok = m_service.post_send(p_channel, msg, 2000);
        if ( !isok ) {
            printf("[TRACE] Test ClientConnectHandler post send fail\n");
            return rpc::RPC_Constants::Fail;
        }
        return rpc::RPC_Constants::Ok;
    }
};

void * run_client(void *)
{
    rpc::RPC_Service<> client;
    client.set_conn_handler(ClientConnectHandler(client));
    bool isok = client.open_channel(RPC_LOCAL_ENDPOINT, 3000);
        
    int ret = client.run_once();
    printf("Client Exit\n");
    return nullptr;
}