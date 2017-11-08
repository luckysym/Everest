#ifndef INCLUDE_EVEREST_NET_IMPL_ASYNC_IO_SERVICE_INL
#define INCLUDE_EVEREST_NET_IMPL_ASYNC_IO_SERVICE_INL

#pragma once
#include <assert.h>

namespace everest 
{
namespace net 
{

template<class F, class E>
inline AsyncIoServiceT<F, E>::AsyncIoServiceT()
    : m_pEvent(new IoEventService)
{
    // TODO
}

template<class F, class E>
inline AsyncIoServiceT<F, E>::~AsyncIoServiceT()
{
    if ( m_pEvent ) {
        delete m_pEvent;
        m_pEvent = nullptr;
    }
    // TODO
}

template<class F, class E>
inline int AsyncIoServiceT<F, E>::SendAsync(int channelId, const char * buf, size_t length, int timeout)
{
    typename BaseType::IoObject * p = BaseType::GetIoObject(channelId);
    if ( p == nullptr ) return -1;
    if ( p->type != BaseType::Type_Channel ) return -2;
    
    return m_pEvent->PostWrite(p->data, ConstBuffer(buf, length), timeout);
}



} // end of namespace net
} // end of namespace everest


#endif // INCLUDE_EVEREST_NET_IMPL_ASYNC_IO_SERVICE_INL