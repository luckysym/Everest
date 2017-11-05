#include <iostream>
#include <everest/net/io_service.h>


int g_result = 0;

#define CHECK( x ) \
    do {\
        g_result = (x)?0:-1; \
        if ( g_result < 0 ) return g_result; \
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



int main(int argc, char **argv)
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

    return g_result;
}
