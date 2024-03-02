/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/memorypool/rawblock.h
  *
  *     Base class for memory blocks of memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMEMORYPOOLMEMORYBLOCK_H
#define HATNMEMORYPOOLMEMORYBLOCK_H

#include <algorithm>
#include <type_traits>
#include <functional>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

//! Raw data block in memory pool
class RawBlock final
{
    public:

        //! Ctor
        RawBlock()=default;

        ~RawBlock()=default;
        RawBlock(const RawBlock&)=delete;
        RawBlock(const RawBlock&&) =delete;
        RawBlock& operator =(const RawBlock&)=delete;
        RawBlock& operator =(RawBlock&&) =delete;

        //! Get data pointer
        inline char* data() noexcept
        {
            return (reinterpret_cast<char*>(this)+sizeof(RawBlock));
        }
        //! Get const data pointer
        inline const char* dataConst() const noexcept
        {
            return (reinterpret_cast<const char*>(this)+sizeof(RawBlock));
        }

        //! Get bucket
        inline void* bucket() const noexcept
        {
            return *const_cast<RawBlock*>(this)->bucketPtr();
        }

        //! Reset bucket
        inline void resetBucket() noexcept
        {
            *bucketPtr()=nullptr;
        }

        inline void setBucket(void* bucket) noexcept
        {
            *bucketPtr()=bucket;
        }

    private:

        // macos clang aligns std::function as 3*sizeof(void*), so 8 bytes are not enough sometimes
        std::aligned_storage_t<(std::max)(sizeof(std::function<void()>),static_cast<size_t>(8))> m_buf;
        void** bucketPtr()
        {
            return reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(&m_buf)+(sizeof(m_buf)-sizeof(void*)));
        }
};

//---------------------------------------------------------------
        } // namespace memory
    HATN_COMMON_NAMESPACE_END
#endif // HATNMEMORYPOOLMEMORYBLOCK_H
