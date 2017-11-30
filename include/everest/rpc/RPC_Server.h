#ifndef INCLUDE_EVEREST_RPC_RPC_SERVER_H
#define INCLUDE_EVEREST_RPC_RPC_SERVER_H

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <memory>
#include <unordered_set>
#include <boost/asio.hpp>
    
    
namespace everest
{
namespace rpc 
{
    struct Tcp_v4
    {
        typedef boost::asio::ip::tcp::acceptor Acceptor;
        typedef boost::asio::ip::tcp::socket   Socket;
        typedef boost::asio::ip::tcp::endpoint Endpoint;
        typedef boost::asio::ip::address_v4    Address;
    };

    template<class Protocol = Tcp_v4>
    class RPC_SocketServer final
    {
    private:
        RPC_SocketServer(const RPC_SocketServer&) = delete;
        RPC_SocketServer& operator=(const RPC_SocketServer&) = delete;
        
    public:
        typedef Protocol  ProtocolType;
        typedef typename Protocol::Acceptor                   Acceptor;
        typedef typename Protocol::Address                    Address;
        typedef typename Protocol::Endpoint                   Endpoint;
        typedef std::shared_ptr<typename Protocol::Acceptor>  AcceptorPtr;
        
    private:
        boost::asio::io_service         m_ioservice;
        std::unordered_set<AcceptorPtr> m_setAcceptor;
        
    public:
        RPC_SocketServer();
        ~RPC_SocketServer();

        int add_listener(const char * endpoint);
        
    }; // end of class RPC_Server
    
} // end of namespace rpc 
} // end of namespace everest 

namespace everest
{
namespace rpc 
{
    template<class Protocol>
    RPC_SocketServer<Protocol>::RPC_SocketServer() {}
    
    template<class Protocol>
    RPC_SocketServer<Protocol>::~RPC_SocketServer() {}
    
    template<class Protocol>
    typename RPC_SocketServer<Protocol>::AcceptorPtr
    RPC_SocketServer<Protocol>::add_listener(const char *endpoint) 
    {
        char * pos = strchr(endpoint, ':');
        if ( pos == nullptr ) return AcceptorPtr(nullptr);
        char * addr = strndup(endpoint, pos - endpoint);
        assert(addr);
        
        Address  address;
        boost::system::error_code ec;
        address.from_string(addr, ec);
        free(addr);
        if ( ec ) return -2;
        int port = atoi(pos + 1);
        
        Endpoint endpoint(address, port);
        AcceptorPtr acceptorPtr(new Acceptor(m_ioservice));
        acceptorPtr->bind(endpoint, ec);
        if ( ec ) return -3;
        
        acceptorPtr->listen(boost::asio::socket_base::max_connections, ec);
        if ( ec ) return -4;
        
        m_setAcceptor.insert();
        
        return 0;
    }
    
} // end of namespace rpc 
} // end of namespace everest 

    

#endif // INCLUDE_EVEREST_RPC_RPC_SERVER_H
