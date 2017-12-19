#ifndef INCLUDE_EVEREST_BUFFER_H
#define INCLUDE_EVEREST_BUFFER_H

#pragma once 
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <list>

namespace everest 
{
    template<class T, class Traits>
    class Basic_Mutable_Buffer
    {
    public:
        typedef T      Value_Type;
        typedef Traits Traits_Type;
        
    private:
        T *    m_buffer;
        size_t m_capacity; // 缓存总长
        size_t m_limit;    // 读写最大长度
        size_t m_size;     // 有效数据长度
        size_t m_pos;      // 当前读写位置
        
    public: 
        Basic_Mutable_Buffer(T *data, size_t capacity) 
            : m_buffer(data), m_capacity(capacity)
            , m_limit(capacity), m_size(0), m_pos(0)
        {
            printf("[TRACE] Basic_Mutable_Buffer init\n");
        }
        
        size_t capacity() const { return m_capacity; }
        
        size_t limit() const { return m_limit; }
        
        size_t limit(size_t s) {
            if ( s > m_capacity ) m_limit = m_capacity;
            else m_limit = s;
            return m_limit;
        }
        
        size_t size() const { return m_size; }
        
        bool   size(size_t newsize) {
            if ( newsize <= m_limit ) {
                m_size = newsize;
                return true; 
            } else return false;
        }
        
        size_t position() const { return m_pos; }
        
        bool position(size_t pos ) { 
            if( pos > m_size) return false;
            m_pos = pos;
            return true;
        }
        
        template<class D = T>
        D * ptr() { return m_buffer; }
        
        template<class D = T>
        const D * ptr() const { return m_buffer; }
        
        template<class D = T>
        D * ptr(size_t offset) { return (D *)(m_buffer + offset); }
        
        template<class D = T>
        const D * ptr(size_t offset) const { return (const D *)(m_buffer + offset); }

        template<class D>
        D & data(size_t offset) { return *(D *)(m_buffer + offset); }
        
        template<class D>
        const D & data(size_t offset) const { return *(const D *)(m_buffer + offset); }
        
        T & operator[](size_t offset) { return m_buffer[offset]; }
        
        const T & operator[](size_t offset) const { return m_buffer[offset]; }
        
    }; // end of class Basic_Mutable_Buffer
    
    template<class Buffer>
    class Basic_Buffer_Sequence
    {
    public:
        typedef std::list<Buffer>           SequenceType;
        typedef typename std::list<Buffer>::iterator Iterator;
        
    private:
        SequenceType m_buffers;
        Iterator     m_it_latest;
        
    public:
        Basic_Buffer_Sequence() {
            m_it_latest = m_buffers.end();
        }
    
        bool push_back(const Buffer &buf) {    // 追加一个缓存
            m_buffers.push_back(buf);
            if ( m_it_latest == m_buffers.end() ) {
                m_it_latest = (--m_buffers.end());
            }
            return true;
        }
        
        bool pop_front() {               // 删除前部一个缓存
            //if ( !m_buffers.empty() ) {
            //    if ( m_it_latest == m_buffers.begin() ) 
            //        ++m_it_latest;
            //    m_buffers.pop_front();
            //}
            return true;
        }
        
        Iterator begin() {               // 获取首个缓存迭代器
            return m_buffers.begin();
        }
        
        Iterator end() {                 // 获取结束缓存迭代器
            return m_buffers.end();
        }
        
        Iterator latest() {              // 最近一次写入的缓存 
            // m_it_latest = m_buffers.end();
            return m_it_latest;
        }
        
        bool latest(Iterator &it) {      // 设置最近一次读取或写入的缓存
            m_it_latest = it;
            return true;
        }
        
        Buffer & front() {               // 首个缓存
            return m_buffers.front();
        }
    }; // end of class Basic_Buffer_Sequence
    
    class Char_Traits {};
    
    typedef Basic_Mutable_Buffer<char, Char_Traits>     Mutable_Byte_Buffer;
    typedef Basic_Buffer_Sequence<Mutable_Byte_Buffer>  Mutable_Buffer_Sequence;
    
} // end of namespace everest 


#endif // INCLUDE_EVEREST_BUFFER_H