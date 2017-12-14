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
        size_t m_capacity;
        
    public: 
        Basic_Mutable_Buffer(size_t capacity) 
            : m_capacity(capacity)
        {
            m_buffer = (T*) ::malloc(sizeof(T) * capacity);
            assert(m_buffer);
            printf("[TRACE] Basic_Mutable_Buffer init\n");
        }
        
        T * ptr() { return m_buffer; }
        const T * ptr() const { return m_buffer; }
        
        size_t capacity() const { return m_capacity; }
        
    }; // end of class Basic_Mutable_Buffer
    
    class Char_Traits {};
    
    typedef Basic_Mutable_Buffer<char, Char_Traits> Mutable_Byte_Buffer;
    
    
} // end of namespace everest 


#endif // INCLUDE_EVEREST_BUFFER_H