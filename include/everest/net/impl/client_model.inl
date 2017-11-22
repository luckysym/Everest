#ifndef INCLUDE_EVEREST_NET_IMPL_CLIENT_MODEL_INL
#define INCLUDE_EVEREST_NET_IMPL_CLIENT_MODEL_INL

#pragma once
#include <assert.h>

namespace everest 
{
namespace net 
{

template<class Cp, class Io>
ClientModel<Cp, Io>::ClientModel(ChannelPool &rChannelPool, IoService &rIoService)
    : m_rChannelPool(rChannelPool)
    , m_rIoService(rIoService)
{}

template<class Cp, class Io>
ClientModel<Cp, Io>::~ClientModel() 
{}

template<class Cp, class Io>
int ClientModel<Cp, Io>::Send(
    int channelId, const char * buf, int length, int timeout)
{
    ChannelType ch = m_rChannelPool.GetChannel(channelId);
    if ( ch == ChannelPool::InvalidChannel ) return -1;
    
    return m_rIoService.PostWrite(ch, ConstBuffer(buf, length), timeout);
}

template<class Cp, class Io>
int ClientModel<Cp, Io>::SendSome(
    int channelId, const char * buf, int length)
{
    ChannelType ch = m_rChannelPool.GetChannel(channelId);
    if ( ch == ChannelPool::InvalidChannel ) return -1;
    
    return m_rIoService.PostWrite(ch, ConstBuffer(buf, length), 0);
}

template<class Cp, class Io>
int ClientModel<Cp, Io>::Receive(
    int channelId, char * buf, int length, int timeout)
{
    ChannelType ch = m_rChannelPool.GetChannel(channelId);
    if ( ch == ChannelPool::InvalidChannel ) return -1;
    
    return m_rIoService.PostWrite(ch, MutableBuffer(buf, length), timeout);
}

template<class Cp, class Io>
int ClientModel<Cp, Io>::ReceiveSome(
    int channelId, char * buf, int length)
{
    ChannelType ch = m_rChannelPool.GetChannel(channelId);
    if ( ch == ChannelPool::InvalidChannel ) return -1;
    
    return m_rIoService.PostWrite(ch, MutableBuffer(buf, length), 0);
}

template<class Cp, class Io>
int ClientModel<Cp, Io>::Wait(int *parrTaskId, int * parrResults, int length)
{
    return m_rIoService.Wait(parrTaskId, parrResults, length);
}



} // end of namespace net
} // end of namespace everest


#endif // INCLUDE_EVEREST_NET_IMPL_CLIENT_MODEL_INL
