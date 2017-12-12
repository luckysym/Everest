#ifndef INCLUDE_EVEREST_DATE_TIME_H
#define INCLUDE_EVEREST_DATE_TIME_H

#pragma once 

#include <stdint.h>
#include <assert.h>
#include <sys/time.h>

namespace everest
{
    class DateTime
    {
    public:
        static int64_t get_timestamp() {
            struct timeval tv;
            int ret = ::gettimeofday(&tv, nullptr);
            assert(ret == 0);
            return (int64_t)(tv.tv_sec * 1000000LL + tv.tv_usec);
        } // end of get_timestamp
    }; // end of DateTime 
    
} // end of namespace everest 

#endif // INCLUDE_EVEREST_DATE_TIME_H
