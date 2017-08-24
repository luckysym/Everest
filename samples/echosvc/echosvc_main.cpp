#include <everest/application.h>
#include <everest/socket.h>
#include <errno.h>
#include <string.h>

namespace e = everest;
namespace s = everest::sock;

int echosvc_main(e::ApplicationContext & ctx);
int run_sync_client(e::ApplicationContext & ctx);
int run_sync_server(e::ApplicationContext & ctx);

int main(int argc, char **argv) 
{
    e::Application app(argc, argv);
    app.run(echosvc_main);
    return app.result();
}

int echosvc_main(e::ApplicationContext & ctx)
{
    e::Properties & props = ctx.properties();
    
    bool is_client = props.exist("client");
    bool is_sync   = props.exist("sync");
    
    if ( is_client ) {
        if ( is_sync ) {
            printf("[info] run sync client\n");
            return run_sync_client(ctx);
        } else {
            printf("[error] run async client: unsupported\n");
        }
    } else {
        if ( is_sync ) {
            printf("[info] run sync server\n");
            return run_sync_server(ctx);
        } else {
            printf("[error] run async server: unsupported\n");
        }
    }
    
    return 0;
}

// 同步客户端
int run_sync_client(e::ApplicationContext & ctx)
{
    e::Properties & props = ctx.properties();
    e::String remote_addr = props.get("addr");
    e::String remote_port = props.get("port");
    e::String count_str   = props.get("count");  // 发送次数
    
    if ( remote_addr.empty() ) {
        printf("[error] --addr not specified\n");
        return -1;
    }
    if ( remote_port.empty() ) {
        printf("[error] --port not specified\n");
        return -1;
    }
    
    s::TcpAddress  address(remote_addr.c_str(), atoi(remote_port.c_str()));
    s::TcpSocket   sock;
    
    bool isok = sock.open(address);
    if ( !isok ) {
        printf("[error] socket open failed, %s:%s\n", remote_addr.c_str(), remote_port.c_str());
        return -1;
    }
    
    int count = atoi(count_str.c_str());
    if ( count == 0 ) count = 1;
    
    for( int i = 0; i < count; ++i ) {
        char buf[128];
        int len = snprintf(buf, 128, "message %d", i);
        ssize_t ret = sock.send(buf, len);
        if ( ret < 0 ) {
            printf("[error] send message failed, %d\n", i);
            break;
        }
        printf("sent message: %s\n", buf);
        
        char rbuf[128];
        ret = sock.recv(rbuf, 128);
        if ( ret < 0 ) {
            printf("[error] recv message failed, %d\n", i);
            break;
        }
        rbuf[ret] = '\0';
        printf("recv message: %s\n", rbuf);
        
    } // end for 
    
    sock.close();
    return 0;
} // end of run_sync_client()

int run_sync_server(e::ApplicationContext &ctx)
{
    e::Properties & props = ctx.properties();
    e::String addr = props.get("addr");
    e::String port = props.get("port");
    e::String backlog = props.get("backlog");
    
    if ( port.empty() ) {
        printf("[error] --port not specified\n");
        return -1;
    }
    
    s::TcpAddress      address(addr.c_str(), atoi(port.c_str()));
    s::TcpServerSocket servsock;
    
    bool isok = servsock.bind(address);
    if ( !isok ) {
        printf("[error] bind server socket failed: %s\n", strerror(errno));
        return -1;
    }
    
    int n_backlog = atoi(backlog.c_str());
    if ( n_backlog == 0 ) n_backlog = 5;
    isok = servsock.open(n_backlog);
    if ( !isok ) {
        printf("[error] open server socket failed, %s\n", strerror(errno));
        return -1;
    }
    
    while ( !ctx.signal_stop() ) {  // 是否有停止信号
        s::TcpAddress remote;
        s::TcpSocket sock( servsock.accept(remote) );
        if ( !sock.valid() ) break;
        
        while ( !ctx.signal_stop() ) {
            char rbuf[128];
            ssize_t ret = sock.recv(rbuf, 128);
            if ( ret < 0 ) {
                printf("[error] recv message failed\n");
                break;
            } else if ( ret == 0 ) {
                printf("[info] remote closed\n");
                break;
            }
            rbuf[ret] = '\0';
            printf("recv message: %s\n", rbuf);
            
            ret = sock.send(rbuf, ret);
            if ( ret < 0 ) {
                printf("[error] send message failed\n");
                break;
            }
            printf("sent message: %s\n", rbuf);
        }
    } // end while
    
    servsock.close();
    return 0;
} // end of run_sync_server()
