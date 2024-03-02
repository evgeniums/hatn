/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/memblocksize.h
  *
  *     Generator of block sizes for memory used in pools and byte arrays
  *
  */

/****************************************************************************/

#ifndef HATNMEMBLOCKSIZE_H
#define HATNMEMBLOCKSIZE_H

#include <iostream>

#include <functional>
#include <map>
#include <memory>
#include <set>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/types.h>
#include <hatn/common/classuid.h>
#include <hatn/common/sharedlocker.h>

#include <hatn/common/memorypool/pool.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

/**
 * @brief Calculator of block sizes for memory used in pools and byte arrays
 */
class HATN_COMMON_EXPORT MemBlockSize
{
    public:

        static size_t DEFAULT_MIN_BLOCK_SIZE; // 32
        static size_t DEFAULT_MAX_BLOCK_SIZE; // 64*1024

        /**
         * @brief Constructor
         * @param minBlockSize Minimum useful size of pool's chunk
         * @param maxBlockSize Maximum useful size of pool's chunk
         */
        MemBlockSize(
            size_t minBlockSize=DEFAULT_MIN_BLOCK_SIZE,
            size_t maxBlockSize=DEFAULT_MAX_BLOCK_SIZE
        ) : m_minBlockSize(minBlockSize),
            m_maxBlockSize(maxBlockSize)
        {
            setMinMaxSizes(minBlockSize,maxBlockSize);
        }

        /**
         * @brief Set limits of useful block size
         * @param minBlockSize Minimum useful size of pool's chunk
         * @param maxBlockSize Maximum useful size of pool's chunk
         */
        inline void setMinMaxSizes(
            size_t minBlockSize,
            size_t maxBlockSize
        )
        {
            Assert(m_minBlockSize!=0,"Min size in MemBlockSize can not be zero");

            m_minBlockSize=minBlockSize;
            m_maxBlockSize=maxBlockSize;

            m_sizes.clear();

            size_t from=1;
            for (size_t to=m_minBlockSize;to<m_maxBlockSize;)
            {
                bool add1600=from<1600;
                bool add4160=from<4160;
                bool add16512=from<16512;
                bool add33024=from<33024;

                KeyRange key={from,to};
                m_sizes.insert(key);
                from=to+1;
                to=nextSize(to);

                if (add1600 && to>1600)
                {
                    key={from,1600};
                    m_sizes.insert(key);
                    from=1600+1;
                    to=nextSize(1600);
                }

                if (add4160 && to>4160)
                {
                    key={from,4160};
                    m_sizes.insert(key);
                    from=4160+1;
                    to=nextSize(4160);
                }

                if (add16512 && to>16512)
                {
                    key={from,16512};
                    m_sizes.insert(key);
                    from=16512+1;
                    to=nextSize(16512);
                }

                if (add33024 && to>33024)
                {
                    key={from,33024};
                    m_sizes.insert(key);
                    from=33024+1;
                    to=nextSize(33024);
                }
            }

            KeyRange key={from,m_maxBlockSize};
            m_sizes.insert(key);
#if 0
            for (auto&& it:m_sizes)
            {
                std::cerr<<it.to<<std::endl;
            }
#endif
        }

        inline size_t minSizeOfBlock() const noexcept
        {
            return m_minBlockSize;
        }
        inline size_t maxSizeOfBlock() const noexcept
        {
            return m_maxBlockSize;
        }
        inline KeyRange keyForSize(size_t size) const noexcept
        {
            KeyRange key={size,size};
            auto it=m_sizes.find(key);
            if (it!=m_sizes.end())
            {
                key=*it;
            }
            return key;
        }

        const std::set<KeyRange>& sizes() const noexcept
        {
            return m_sizes;
        }

    private:

        std::set<KeyRange> m_sizes;

        size_t m_minBlockSize;
        size_t m_maxBlockSize;

        inline size_t nextSize(size_t size) noexcept
        {
            size_t s=size+(size>>1); // multiply 1.5
            return 32u+((s>>5)<<5); // align to 32
        }
};

//---------------------------------------------------------------
} // namespace memorypool
HATN_COMMON_NAMESPACE_END
#endif // HATNMEMBLOCKSIZE_H
