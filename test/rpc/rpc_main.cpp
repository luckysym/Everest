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
public:
    int operator()(rpc::RPC_SocketListener *p_listener, rpc::RPC_SocketChannel * p_channel, int ec)
    {
        printf("[ERROR] Test ServerAcceptHandler not impl, %p, %p, %d\n", p_listener, p_channel, ec);
        return rpc::RPC_Constants::Fail;
    }
};



void * run_server(void *)
{
    rpc::RPC_Service<> server;
    
    server.set_accept_handler(ServerAcceptHandler());
    
    rpc::RPC_Service<>::ListenerPtr ptrListener= server.open_listener(RPC_LOCAL_ENDPOINT);
    if ( !ptrListener ) { throw std::runtime_error("open listener failed"); } 
    
    bool isok = server.post_accept(ptrListener, 5000);
    if ( !isok ) { throw std::runtime_error("post accept failed");}
    
    int result = server.run();
    if ( result < 0 ) { throw std::runtime_error("server run failed");}
    
    printf("Server Exit, result %d\n", result);
    return nullptr;
}

void * run_client(void *)
{
    rpc::RPC_Service<> client;
    bool isok = client.open_channel(RPC_LOCAL_ENDPOINT, 3000);
    
    int ret = client.run();
    printf("Client Exit\n");
    return nullptr;
}