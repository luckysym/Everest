#ifndef INCLUDE_EVEREST_NET_IO_SERVICE_H
#define INCLUDE_EVEREST_NET_IO_SERVICE_H

#pragma once
#include <everest/properties.h>
#include <vector>
#include <memory>

namespace everest 
{
namespace net 
{
    class ConstBuffer
    {
    private:
        const char * m_pBufferRef;
        size_t       m_nLength;

    public:
        ConstBuffer() : m_pBufferRef(nullptr), m_nLength(0) {}

        ConstBuffer(const char * buf, size_t len) 
            : m_pBufferRef(buf)
            , m_nLength(len) {}

        ConstBuffer(const ConstBuffer& cBuffer) 
            : m_pBufferRef(cBuffer.m_pBufferRef)
            , m_nLength(cBuffer.m_nLength) {}

        ~ConstBuffer() { m_pBufferRef = nullptr; m_nLength = 0;}
        
        ConstBuffer & operator= (const ConstBuffer& cBuffer) {
            if ( this == &cBuffer ) return *this;
            m_pBufferRef = cBuffer.m_pBufferRef;
            m_nLength = cBuffer.m_nLength;
            return *this;
        }

        size_t Length() const { return m_nLength; }
        const char * Ptr() const { return m_pBufferRef; }

    }; // end of class ConstBuffer

    class MutableBuffer
    {
    private:
        char * m_pBufferRef;
        size_t m_nCapacity;    // 缓存总长
        size_t m_nLength;      // 有效数据长度

    public:
        MutableBuffer(char * buf, size_t capacity, size_t len = 0 ) 
            : m_pBufferRef(buf)
            , m_nCapacity(capacity)
            , m_nLength(len) {}

        MutableBuffer(const MutableBuffer& cBuffer) 
            : m_pBufferRef(cBuffer.m_pBufferRef)
            , m_nCapacity(cBuffer.m_nCapacity)
            , m_nLength(cBuffer.m_nLength) {}

        ~MutableBuffer() {
            m_pBufferRef = nullptr; 
            m_nCapacity = 0; 
            m_nLength = 0; 
        }

        MutableBuffer & operator= (const MutableBuffer& cBuffer) {
            if ( this == &cBuffer ) return *this;
            m_pBufferRef = cBuffer.m_pBufferRef;
            m_nCapacity = cBuffer.m_nCapacity;
            m_nLength = cBuffer.m_nLength;
            return *this;
        }

        size_t Capacity() const { return m_nCapacity; }
        size_t Length() const { return m_nLength; }
        char * Ptr() const { return m_pBufferRef; }

    }; // end of class MutableBuffer

    /**
     * 网络通讯客户端模型
     */
    template<class Cp, class Io>
    class ClientModel
    {
    private:
        ClientModel(const ClientModel& );
        ClientModel& operator=(const ClientModel& );
    
    public:
        typedef Cp   ChannelPool;
        typedef Io   IoService;
        typedef typename ChannelPool::ChannelType ChannelType;

    private:
        ChannelPool & m_rChannelPool;
        IoService   & m_rIoService;

    public:
        ClientModel(ChannelPool &rChannelPool, IoService &rIoService);
        ~ClientModel();

        ssize_t Send(int channelId, const char * buf, size_t length, int timeout);
        ssize_t SendSome(int channelId, const char * buf, size_t length);

        ssize_t Receive(int channelId, char *buf, size_t length, int timeout);
        ssize_t ReceiveSome(int channelId, char *buf, size_t length);

    }; // end of class ClientModel

    /**
     * 网络通讯服务端模型
     */ 
    class ServerModel
    {
    public:
    }; // end of class ServerModel




} // end of namespace net 
} // end of namespace everest

#include <everest/net/impl/client_model.inl>

#endif // INCLUDE_EVEREST_NET_IO_SERVICE_H
