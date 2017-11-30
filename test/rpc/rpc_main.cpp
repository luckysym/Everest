#include <everest/rpc/RPC_Server.h>
#include <iostream>

namespace rpc = everest::rpc;

#define CHECK( x ) \
    do {\
        if ( !(x) ) return -1; \
    } while (0) 

        
int run_server(int argc, char **argv);
        
int main(int argc, char **argv)
{
    return run_server(argc, argv);
}



int run_server(int argc, char **argv)
{
    using namespace std;
    rpc::RPC_SocketServer<> server;
    
    int acceptorId = server.add_listener("127.0.0.1:8090");
    if ( acceptorId < 0) {
        cout<<"[error] add listener failed"<<endl;
        return -1;
    }
    
    return 0;
}
