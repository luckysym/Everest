#ifndef INCLUDE_EVEREST_RPC_RPC_MESSAGE_H
#define INCLUDE_EVEREST_RPC_RPC_MESSAGE_H

#pragma once 

#include <everest/buffer.h>

namespace everest 
{
namespace rpc 
{
    class RPC_Message 
    {
    public:
        static const size_t Header_Length = 24;
        
        // Message Type
        static const char   Type_Request  = 0;
        
        // Message field index
        static const size_t Idx_Length = 8;
        
        // byte endian
        static const char   Little_Endian = 0;
        static const char   Big_Endian    = 1; 
        
    public:
        typedef Mutable_Buffer_Sequence Buffer_Sequence;
        
    private:
        Buffer_Sequence *m_buffer_seq;
        
    public:
        RPC_Message () : m_buffer_seq(nullptr) {}
  
        RPC_Message(Mutable_Buffer_Sequence &buffers) 
        {
            m_buffer_seq = &buffers;
        }
        
        ~RPC_Message() {}
        
        bool init_header();
        bool update_header();
        bool add_buffer(Mutable_Byte_Buffer & buf); 
        
        size_t size() const ;  // 获取消息长度
        
        Buffer_Sequence * detach_buffers();
        Buffer_Sequence &buffers() { return *m_buffer_seq; }
    }; // end of class RPC_Message 
    
    RPC_Message::Buffer_Sequence * RPC_Message::detach_buffers()
    {
        auto p = m_buffer_seq;
        m_buffer_seq = nullptr;
        return p;
    }
    
    size_t RPC_Message::size() const 
    {
        Buffer_Sequence::Iterator it = m_buffer_seq->begin();
        if ( it == m_buffer_seq->end() ) {
            printf("[ERROR] RPC_Message::size, no buffer\n");
            throw std::runtime_error("RPC_Message::size, no buffer");
        }
        if ( it->size() < Header_Length) {
            printf("[ERROR] RPC_Message::size, message is too short\n");
            throw std::runtime_error("RPC_Message::size, message is too short");
        }
        return (size_t)it->data<uint32_t>(Idx_Length);
    } // end of RPC_Message::size()
    
    bool RPC_Message::update_header()
    {
        uint32_t total_len = 0;
        Buffer_Sequence::Iterator it = m_buffer_seq->begin();
        for(; it != m_buffer_seq->end(); ++it) {
            total_len += it->size();
        }
        m_buffer_seq->front().data<uint32_t>(Idx_Length) = total_len;
    }
    
    bool RPC_Message::add_buffer(Mutable_Byte_Buffer & buf) {
        m_buffer_seq->push_back(buf);
        return true;
    }
    
    bool RPC_Message::init_header() {
        Buffer_Sequence::Iterator it = m_buffer_seq->begin();
        if ( it == m_buffer_seq->end() ) {
            printf("[ERROR] RPC_Message::init_header, no buffer\n");
            return false;
        }
        Mutable_Byte_Buffer &buf = *it;
        bool isok = buf.size(Header_Length);
        if ( !isok ) {
            printf("[ERROR] RPC_Message::init_header, no enough space\n");
            return false;
        }
        
        // magic
        buf[0] = 'G'; buf[1] = 'I'; buf[2] = 'O'; buf[3] = 'P';
        // version
        buf[4] = '0'; buf[5] = '0';  
        // flags/byte order and message type 
        buf[6] = Little_Endian; buf[7] = Type_Request;
        // length
        buf.data<uint32_t>(8) = 0;
        
        return true;
    } // end of RPC_Message::init_header

} // end of namespace rpc 
} // end of namespace everest
    
#endif // INCLUDE_EVEREST_RPC_RPC_MESSAGE_H
