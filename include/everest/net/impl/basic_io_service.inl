#ifndef INCLUDE_EVEREST_NET_IMPL_BASIC_IO_SERVICE_INL
#define INCLUDE_EVEREST_NET_IMPL_BASIC_IO_SERVICE_INL

#pragma once
#include <assert.h>

namespace everest 
{
namespace net 
{

template<class F>
inline BasicIoServiceT<F>::BasicIoServiceT()
    : m_pFactory(new IoFactory)
{
    m_vecObjects.reserve(Default_Max_Size);
    // TODO
} // end of BasicIoServiceT<F>::BasicIoServiceT()

template<class F>
inline BasicIoServiceT<F>::~BasicIoServiceT()
{
    if ( m_pFactory ) {
        delete m_pFactory; 
        m_pFactory = nullptr;
    }
} // end of BasicIoServiceT<F>::~BasicIoServiceT()

template<class F>
inline int BasicIoServiceT<F>::OpenChannel(const char *endpoint)
{
    IoObjectType object = m_pFactory->OpenChannel(endpoint);
    if ( m_pFactory->IsValid(object) ) {
        m_vecObjects.push_back(IoObject{Type_Channel, object});
        return m_vecObjects.size() - 1;
    } else {
        return -1;
    }
}

template<class F>
inline int BasicIoServiceT<F>::OpenServer(const char *endpoint)
{
    IoObjectType object = m_pFactory->OpenServer(endpoint);
    if ( m_pFactory->IsValid(object) ) {
        m_vecObjects.push_back(IoObject{Type_Server, object});
        return m_vecObjects.size() - 1;
    } else {
        return -1;
    }
}

template<class F>
inline bool BasicIoServiceT<F>::CloseChannel(int objectId)
{
    if ( objectId >=0 && objectId < m_vecObjects.size() ) 
    {
        IoObject & obj = m_vecObjects[objectId];
        if ( obj.type == Type_Channel) {
            obj.type = Type_Unknown;
            return m_pFactory->CloseChannel(obj.data);
        } else {
            return false;
        }
    }
}

template<class F>
inline bool BasicIoServiceT<F>::CloseServer(int objectId)
{
    if ( objectId >=0 && objectId < m_vecObjects.size() ) 
    {
        IoObject & obj = m_vecObjects[objectId];
        if ( obj.type == Type_Server) {
            obj.type = Type_Unknown;
            return m_pFactory->CloseServer(obj.data);
        } else {
            return false;
        }
    }
}

template<class F>
inline bool BasicIoServiceT<F>::CloseAll()
{
    for(int i = 0; i < m_vecObjects.size(); ++i) {
        IoObject & obj = m_vecObjects[i];
        if ( obj.type == Type_Server) {
            obj.type = Type_Unknown;
            m_pFactory->CloseServer(obj.data);
        } else if (obj.type == Type_Channel) {
            obj.type = Type_Unknown;
            m_pFactory->CloseChannel(obj.data);
        } 
    }
    return true;
}

} // end of namespace net
} // end of namespace everest 


#endif // INCLUDE_EVEREST_NET_IMPL_BASIC_IO_SERVICE_INL