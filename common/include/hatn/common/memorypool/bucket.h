/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/memorypool/bucket.h
  *
  *     Base class for chunk buckets of memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMEMORYPOOLBUCKET_H
#define HATNMEMORYPOOLBUCKET_H

#include <stddef.h>
#include <tuple>

#include <hatn/common/common.h>
#include <hatn/common/objecttraits.h>
#include <hatn/common/memorypool/rawblock.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace memorypool {

template <typename BucketT>
struct BucketTraitsBase
{
    BucketT* m_bucket;
    BucketTraitsBase(BucketT* bucket) : m_bucket(bucket)
    {}
};

//! Memory pool bucket
template <typename PoolT, typename BucketTraits>
class Bucket : public WithTraits<BucketTraits>
{
    public:

        //! Ctor
        template <typename BucketT, typename ... Args>
        Bucket(
            BucketT* bucket,
            PoolT& pool,
            Args&& ...traitsArgs
        ) noexcept :
            WithTraits<BucketTraits>(bucket,std::forward<Args>(traitsArgs)...),
            m_pool(pool)
        {}

        //! Get pool
        inline PoolT& pool() noexcept
        {
            return m_pool;
        }

    protected:

        PoolT& m_pool;
};

//---------------------------------------------------------------
        } // namespace memory
HATN_COMMON_NAMESPACE_END
#endif // HATNMEMORYPOOLBUCKET_H
