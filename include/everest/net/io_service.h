#ifndef INCLUDE_EVEREST_NET_IO_SERVICE_H
#define INCLUDE_EVEREST_NET_IO_SERVICE_H

#pragma once
#include <everest/properties.h>

namespace everest 
{
namespace net 
{
    template<class T>
    class BasicIoServiceT
    {
    public:
        typedef int Id;
        
    private:
        BasicIoServiceT(const BasicIoServiceT&);
        BasicIoServiceT& operator=(const BasicIoServiceT&)

    public:
        BasicIoServiceT();
        ~BasicIoServiceT();

        Id CreateChannel(const Properties& props);
        Id CreateServer(const Properties& props);

        bool CloseChannel(Id id);
        bool CloseServer(Id id);

    }; // end of BasicIoServiceT

    // 异步I/O服务
    template<class T >
    class AsyncIoServiceT 
    {
    public:
        typedef int Id;
        
    private:
        AsyncIoServiceT(const AsyncIoServiceT&);
        AsyncIoServiceT& operator=(const AsyncIoServiceT&)

        // 向指定的channel异步发送指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。
        int SendAsync(Id channelId, const char * buf, size_t length, int timeout);
        
        // 向指定的channel异步发送消息。但消息的内容、超时时间由函数Fn确定，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename SFn>
        int SendAsync(Id channelId, SFn fn);
        
        // 从指定的channel异步接收指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。
        int ReceiveAsync(Id channelId, char * buf, size_t length, int timeout);
                
        // 从指定channel异步接收消息，接收消息的长度和超时时间，由函数Fn确定。返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename RFn>
        int ReceiveAsync(Id channelId, RFn fn);
        
        // 从指定的server异步获取一个连入的Channel，返回此异步任务的ID(>=0). 任务创建失败返回-1.
        int AcceptAsync(Id serverId);

        // 等待异步时间完成，返回已完成的异步时间数，同时输出完成的异步任务ID列表。
        int Wait(int *taskIds, int len);
        
    }; // end of class AsyncIoServiceT

    // 既支持同步，也支持异步的Io服务
    template <class T>
    class IoServiceT : public BasicIoServiceT<T>
    {
    public:
        IoServiceT();
        ~IoServiceT();

        // 向指定的channel同步发消息，全部发送完成后返回true, 或者超时或失败后返回false。
        // timeout单位毫秒。
        bool Send(Id channelId, const char * buf, size_t length, int timeout);

        // 向指定的channel同步发送消息。但消息的内容、超时时间由函数Fn确定。
        // 发送完成后返回true，如果超时或发送失败返回false。
        template<typename SFn>
        bool Send(Id channelId, SFn fn);

        // 从指定的channel同步接收指定长度的消息，消息全部收到返回true，消息接收失败或超时返回false。
        bool Receive(Id channelId, char * buf, size_t length, int timeout);

        // 从指定channel同步接收消息，接收消息的长度和超时时间，由函数Fn确定。
        // 接收完成返回true, 超时或者接收失败返回false.
        template<typename RFn>
        bool Receive(Id channelId, RFn fn);

        // 向指定的channel异步发送指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。
        int SendAsync(Id channelId, const char * buf, size_t length, int timeout);

        // 向指定的channel异步发送消息。但消息的内容、超时时间由函数Fn确定，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示发送成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename SFn>
        int SendAsync(Id channelId, SFn fn);

        // 从指定的channel异步接收指定长度的消息，返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。
        int ReceiveAsync(Id channelId, char * buf, size_t length, int timeout);
        
        // 从指定channel异步接收消息，接收消息的长度和超时时间，由函数Fn确定。返回这次异步任务的标识ID(>= 0)。
        // 异步任务创建失败则返回-1。函数返回并不表示接收成功。函数fn可能被其他线程所调用，视具体实现而定
        template<typename RFn>
        int ReceiveAsync(Id channelId, RFn fn);

        // 同步获取一个连入的channel，并返回channel id。该方法可能会被阻塞
        Id Accept(Id serverId);

        // 从指定的server异步获取一个连入的Channel，返回此异步任务的ID(>=0). 任务创建失败返回-1.
        int AcceptAsync(Id serverId);

    }; // end of class IoServiceT

} // end of namespace net 
} // end of namespace everest


#endif // INCLUDE_EVEREST_NET_IO_SERVICE_H
