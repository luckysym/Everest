#ifndef INCLUDE_EVEREST_NET_IO_SERVICE_H
#define INCLUDE_EVEREST_NET_IO_SERVICE_H

#pragma once
#include <everest/properties.h>
#include <vector>

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
        ConstBuffer(const char * buf, size_t len) 
            : m_pBufferRef(buf), m_nLength(len) {}

        ~ConstBuffer() { m_pBufferRef = nullptr; m_nLength = 0;}

        size_t Length() const { return m_nLength; }
    }; // end of class ConstBuffer

    template<class F>
    class BasicIoServiceT
    {
    public:
        typedef F   IoFactory;       // IO对象工厂类型

        typedef typename IoFactory::IoObjectType  IoObjectType;
        
        static const int    Type_Unknown = 0;
        static const int    Type_Channel = 1;
        static const int    Type_Server  = 2;
        static const size_t Default_Max_Size = 1024;  // 默认最大允许的IO对象实例数

    protected:
        struct IoObject
        {
            int          type;    // 0-channel, 1-server
            IoObjectType data;    // the raw io object type
        };

    private:
        IoFactory * m_pFactory;   // I/O对象工厂，用于创建Channel和Server对象实例。
        std::vector<IoObject>  m_vecObjects;   // Channel和Server对象实例列表

    private:
        BasicIoServiceT(const BasicIoServiceT&);
        BasicIoServiceT& operator=(const BasicIoServiceT&);

    public:
        BasicIoServiceT();
        ~BasicIoServiceT();

        int OpenChannel(const char* endpoint);
        int OpenServer(const char* endpoint);

        bool CloseChannel(int id);
        bool CloseServer(int id);

    protected:
        IoObject * GetIoObject(int id) {
            if ( id >= 0 && id < m_vecObjects.size()) 
                return &m_vecObjects[id];
            else
                return nullptr;
        }

    }; // end of BasicIoServiceT
    
    // 异步I/O服务
    template<class F, class E >
    class AsyncIoServiceT : public BasicIoServiceT<F>
    {
    public:
        typedef BasicIoServiceT<F> BaseType;
        typedef F IoFactory;
        typedef E IoEventService;

    private:
        IoEventService * m_pEvent;

    private:
        AsyncIoServiceT(const AsyncIoServiceT&);
        AsyncIoServiceT& operator=(const AsyncIoServiceT&);

    public:
        AsyncIoServiceT();
        ~AsyncIoServiceT();

        // 向指定的channel异步发送指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。
        int SendAsync(int channelId, const char * buf, size_t length, int timeout);
        
        // 向指定的channel异步发送消息。但消息的内容、超时时间由函数Fn确定，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename SFn>
        int SendAsync(int channelId, SFn fn);
        
        // 从指定的channel异步接收指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。
        int ReceiveAsync(int channelId, char * buf, size_t length, int timeout);
                
        // 从指定channel异步接收消息，接收消息的长度和超时时间，由函数Fn确定。返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename RFn>
        int ReceiveAsync(int channelId, RFn fn);
        
        // 从指定的server异步获取一个连入的Channel，返回此异步任务的ID(>=0). 任务创建失败返回-1.
        int AcceptAsync(int serverId);

        // 等待异步时间完成，返回已完成的异步时间数，同时输出完成的异步任务ID列表。
        int Wait(int *taskIds, int len);
        
    }; // end of class AsyncIoServiceT

    // 既支持同步，也支持异步的Io服务
    template <class F, class T>
    class IoServiceT : public BasicIoServiceT<F>
    {
    public:
        IoServiceT();
        ~IoServiceT();

        // 向指定的channel同步发消息，全部发送完成后返回true, 或者超时或失败后返回false。
        // timeout单位毫秒。
        bool Send(int channelId, const char * buf, size_t length, int timeout);

        // 向指定的channel同步发送消息。但消息的内容、超时时间由函数Fn确定。
        // 发送完成后返回true，如果超时或发送失败返回false。
        template<typename SFn>
        bool Send(int channelId, SFn fn);

        // 从指定的channel同步接收指定长度的消息，消息全部收到返回true，消息接收失败或超时返回false。
        bool Receive(int channelId, char * buf, size_t length, int timeout);

        // 从指定channel同步接收消息，接收消息的长度和超时时间，由函数Fn确定。
        // 接收完成返回true, 超时或者接收失败返回false.
        template<typename RFn>
        bool Receive(int channelId, RFn fn);

        // 向指定的channel异步发送指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。
        int SendAsync(int channelId, const char * buf, size_t length, int timeout);

        // 向指定的channel异步发送消息。但消息的内容、超时时间由函数Fn确定，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename SFn>
        int SendAsync(int channelId, SFn fn);

        // 从指定的channel异步接收指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。
        int ReceiveAsync(int channelId, char * buf, size_t length, int timeout);
        
        // 从指定channel异步接收消息，接收消息的长度和超时时间，由函数Fn确定。返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename RFn>
        int ReceiveAsync(int channelId, RFn fn);

        // 同步获取一个连入的channel，并返回channel id。该方法可能会被阻塞
        int Accept(int serverId);

        // 从指定的server异步获取一个连入的Channel，返回此异步任务的ID(>=0). 任务创建失败返回-1.
        int AcceptAsync(int serverId);

    }; // end of class IoServiceT

} // end of namespace net 
} // end of namespace everest

#include <everest/net/impl/basic_io_service.inl>
#include <everest/net/impl/async_io_service.inl>

#endif // INCLUDE_EVEREST_NET_IO_SERVICE_H
