#ifndef INCLUDE_EVEREST_BUFFER_H
#define INCLUDE_EVEREST_BUFFER_H

#pragma once 
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

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
        size_t m_size;     // 有效数据长度
        
    public: 
        Basic_Mutable_Buffer(T *data, size_t capacity) 
            : m_buffer(data), m_capacity(capacity)
        {
            printf("[TRACE] Basic_Mutable_Buffer init\n");
        }
        
        size_t capacity() const { return m_capacity; }
        
        size_t size() const { return m_size; }
        
        bool resize(size_t newsize) {
            if ( newsize <= m_capacity ) {
                m_size = newsize;
                return true; 
            } else return false;
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
    
    class Char_Traits {};
    
    typedef Basic_Mutable_Buffer<char, Char_Traits> Mutable_Byte_Buffer;
    
    
} // end of namespace everest 


#endif // INCLUDE_EVEREST_BUFFER_H