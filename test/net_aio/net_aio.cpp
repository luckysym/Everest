#include <iostream>
#include <string.h>
#include <everest/net/io_service.h>

namespace en = everest::net;

#define CHECK( x ) \
    do {\
        if ( !(x) ) return -1; \
    } while (0) 

class FakeFactory
{
public:
    typedef int IoObjectType;

public:
    FakeFactory() : m_objectId(0) {}

    IoObjectType OpenChannel(const char * endpoint)
    {
        using namespace std;
        cout<<"FakeFactory::OpenChannel: "<<endpoint<<"; id = "<<m_objectId<<endl;
        return m_objectId++;
    }

    bool CloseChannel(IoObjectType obj) 
    {
        using namespace std;
        cout<<"FakeFactory::CloseChannel: "<< obj <<endl;
        return obj < m_objectId;
    }

    bool CloseServer(IoObjectType obj) 
    {
        using namespace std;
        cout<<"FakeFactory::CloseServer: "<< obj <<endl;
        return obj < m_objectId;
    }

    IoObjectType OpenServer(const char * endpoint)
    {
        using namespace std;
        cout<<"FakeFactory::OpenServer: "<<endpoint<<"; id = "<<m_objectId<<endl;
        return m_objectId++;
    }

    bool IsValid(IoObjectType obj) const 
    {
        return obj < m_objectId;
    }

private:
    int m_objectId;
}; // end of 

class FakeEventService
{
public:
    FakeEventService() : m_taskId(0) {}

    template<class ConstBuf>
    int PostWrite(FakeFactory::IoObjectType obj, const ConstBuf& buf, int timeout)
    {
        using namespace std;
        cout<<"FakeEventService::PostWrite(1): "<<obj<<", bufsize "<<buf.Length()<<", timeout "<<timeout<<endl;
        return m_taskId++;
    }

    template<class FnSendComplete, class ConstBuf>
    int PostWrite(FakeFactory::IoObjectType obj,FnSendComplete fnComplete, void *arg)
    {
        using namespace std;
        cout<<"FakeEventService::PostWrite(2): "<<obj<<endl;
        ConstBuf buf;
        int newTaskId = m_taskId++;
        int ret = fnComplete(newTaskId, buf, 0, arg);
        cout<<"FakeEventService::PostWrite(2): "<<ret<<endl;
        return newTaskId;
    }

private:
    int m_taskId;
}; // end of class FakeEventService

static int test_basic_iosvc_fake(int argc, char **argv)
{
    using namespace everest::net;

    BasicIoServiceT<FakeFactory>  ioservice;
    int channelId = ioservice.OpenChannel("192.168.1.1:80");
    CHECK(channelId >= 0); 
    int serverId  = ioservice.OpenServer("8080");
    CHECK(serverId >= 0);

    bool isok = ioservice.CloseChannel(channelId);
    CHECK(isok);

    isok = ioservice.CloseServer(serverId);
    CHECK(isok);

    return 0;
}

static int test_async_iosvc_fake(int argc, char **argv)
{
    using namespace everest::net;
    using namespace std;
    cout<<"test_async_iosvc_fake"<<endl;
    const char * msg = "hello, world\n";
    AsyncIoServiceT<FakeFactory, FakeEventService> ioservice;
    
    int channelId = ioservice.OpenChannel("192.168.1.1:80");
    cout<<"\topen channel: "<<channelId<<endl;
    CHECK(channelId >= 0);

    int sendTaskId = ioservice.SendAsync(channelId, msg, strlen(msg), 5000);
    CHECK(sendTaskId >= 0);

   
    ioservice.CloseChannel(channelId);
    return 0;
}




int main(int argc, char **argv)
{
    CHECK( 0 == test_basic_iosvc_fake(argc, argv) );
    CHECK( 0 == test_async_iosvc_fake(argc, argv) );
    return 0;
}