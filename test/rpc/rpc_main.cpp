#include <pthread.h>
#include <everest/rpc/RPC_Server.h>
#include <iostream>

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

void * run_server(void *)
{
    using namespace std;
    rpc::RPC_SocketService<> server;
    
    rpc::RPC_SocketService<>::AcceptorPtr acceptorPtr = server.open_listener("127.0.0.1:8090");
    if ( !acceptorPtr ) {
        cout<<"[error] open listener failed"<<endl;
        server_result = -1;
        return nullptr;
    }
    cout<<"[info] listener ok"<<endl;
    return 0;
}

void client_conn_handler(
    rpc::RPC_SocketService<> & service,
    rpc::RPC_SocketService<>::SocketPtr & sock, 
    bool success)
{
    if ( success ) {
        throw std::runtime_error("client conn success");
    } else {
        throw std::runtime_error("client conn fail");
    }
}

void * run_client(void *)
{
    using namespace std;
    rpc::RPC_SocketService<> client;
    
    client.set_conn_handler(client_conn_handler);
    
    rpc::RPC_SocketService<>::SocketPtr socketPtr = client.open_channel("127.0.0.1:8090");
    if ( !socketPtr ) {
        cout<<"[error] open channel failed"<<endl;
        client_result = -1;
        return nullptr;
    }
    client.run();
    cout<<"[info] open client ok"<<endl;
    return nullptr;
}