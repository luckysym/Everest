#include <iostream>
#include <string.h>
#include <everest/net/io_service.h>

namespace en = everest::net;

#define CHECK( x ) \
    do {\
        if ( !(x) ) return -1; \
    } while (0) 

class FakeChannelPool
{
public:
    typedef int ChannelType;
    static const int InvalidChannel = -1;
};

class FakeIoService
{
public:
    template<class Buf>
    ssize_t PostRead(int channel, const Buf &buf, int timeout) {
        return 0;
    }

    template<class Buf>
    ssize_t PostWrite(int channel, const Buf &buf, int timeout) {
        return 0;
    }
};


int test_client_model(int argc, char **argv)
{
    FakeChannelPool pool;
    FakeIoService   iosvc;
    en::ClientModel<FakeChannelPool, FakeIoService> client(pool, iosvc);
    return 0;
}

int main(int argc, char **argv)
{
    CHECK( 0 == test_client_model(argc, argv) );
    return 0;
}