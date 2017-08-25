#include <everest/application.h>
#include <everest/socket.h>
#include <everest/network.h>
#include <errno.h>
#include <string.h>

namespace e = everest;
namespace s = everest::net;

int echosvc_main(e::ApplicationContext & ctx);      
int run_sync_client(e::ApplicationContext & ctx);  // 单线程同步客户端
int run_sync_server(e::ApplicationContext & ctx);  // 单线程同步服务端
int run_async_server(e::ApplicationContext & ctx); // 单线程异步服务端


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
            printf("[info] run async server\n");
            run_async_server(ctx);
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
    
    s::InetAddress address(remote_addr.c_str(), atoi(remote_port.c_str()));
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
        int len = snprintf(buf + 2, 128, "message %d", i);
        *((short *)buf) = (short)len;
        ssize_t ret = sock.send(buf, len + 2);
        if ( ret < 0 ) {
            printf("[error] send message failed, %d\n", i);
            break;
        }
        printf("sent message: %s\n", buf + 2);
        
        char rbuf[128];
        ret = sock.recv(rbuf, 128);
        if ( ret < 0 ) {
            printf("[error] recv message failed, %d\n", i);
            break;
        }
        rbuf[ret] = '\0';
        printf("recv message: (%d) %s\n", *((short*)rbuf), rbuf + 2);
        sleep(1);
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
    
    s::InetAddress     address(addr.c_str(), atoi(port.c_str()));
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
        s::InetAddress remote;
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
            printf("server recv message: (%d) %s\n", *((short*)rbuf), rbuf + 2);
            
            ret = sock.send(rbuf, ret);
            if ( ret < 0 ) {
                printf("[error] send message failed\n");
                break;
            }
            printf("sent message: (%d) %s\n", ret, rbuf + 2);
        }
    } // end while
    
    servsock.close();
    return 0;
} // end of run_sync_server()

int run_async_server(e::ApplicationContext & ctx)
{
    e::Properties & props = ctx.properties();
    e::String addr = props.get("addr");
    e::String port = props.get("port");
    e::String backlog = props.get("backlog");
    
    if ( port.empty() ) {
        printf("[error] --port not specified\n");
        return -1;
    }
    
    s::InetAddress     address(addr.c_str(), atoi(port.c_str()));
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
    
    s::EPoller poller;
    isok = poller.add( servsock.handle(), EPOLLIN, &servsock);
    
    while ( !ctx.signal_stop() ) {
        int ret = poller.wait(1000);
        if ( ret < 0 ) {
            printf("[error] epoll wait failed, %s", strerror(errno));
            break;
        } else if ( ret == 0) {
            continue;
        }
        
        s::EPoller::Iterator it = poller.rbegin();
        for(; it != poller.rend(); ++it ) {
            if ( it->object() == &servsock && it->events() & EPOLLIN ) {
                // new conn
                s::TcpSocket * s = new s::TcpSocket(-1);
                bool is_accept = servsock.accept(*s);
                if  ( !is_accept ) {
                    printf("[error] accept new conn failed, %s", strerror(errno));
                    delete s;
                    continue;
                }
                printf("[info] new conn accepted: %d\n", s->handle());
                isok = poller.add(s->handle(), EPOLLIN, s);
                if  ( !isok ) {
                    printf("[warn] client socket failed to add epoller, %s", strerror(errno));
                    delete s;
                }
                continue;
            } // end servsock accept
            
            if ( it->object() != &servsock ) {
                s::TcpSocket * s = (s::TcpSocket*)it->object();
                assert(s);
                if ( it->events() & EPOLLIN ) {
                    char buf[128];
                    ssize_t ret = s->recv(buf, 128);
                    if ( ret == 0 ) {
                        poller.remove(s->handle());
                        delete s;
                        continue;
                    }
                    printf("recv message %s\n", buf + 2);
                    
                    ret = s->send(buf, ret);
                    printf("send message (ret) %s\n", buf + 2);
                }
            }
        } // end for 
    } // end while 
    
    s::EPoller::Iterator it;
    for ( it = poller.begin(); it != poller.end(); ++it) {
        s::TcpBasicSocket * s = (s::TcpBasicSocket *)it->object();
        if ( s == nullptr) continue;
        poller.remove(s->handle());
        if ( it->object() != &servsock ) {
            delete s;
        }
    }
    
    servsock.close();
    return 0;
} // end of run_async_server()
