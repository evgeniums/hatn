/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pmr/singlepoolmemoryresource.h
  *
  *     Polymorphic memory resource using single memory pool
  *
  */

/****************************************************************************/

#ifndef HATNSINGLEPOOLMEMORYRESOURCE_H
#define HATNSINGLEPOOLMEMORYRESOURCE_H

#include <hatn/common/common.h>
#include <hatn/common/memorypool/pool.h>
#include <hatn/common/memorypool/bucket.h>

#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pmr {

/**
 * @brief Memory resource that uses single memory pool as backend
 *
 * Memory pool must be preset in constructor.
 *
 */
template <typename PoolT>
class SinglePoolMemoryResource final : public memory_resource
{
    public:

        /**
         * @brief Ctor from existing memory pool
         * @param pool Memory pool to use as backend for this resource
         */
        explicit SinglePoolMemoryResource(
            std::shared_ptr<PoolT> pool
        ) :
            m_pool(std::move(pool))
        {
            Assert(m_pool,"Null pool is not allowed");
        }

        /**
         * @brief Get memory pool
         * @return Underlying memory pool
         */
        inline std::shared_ptr<PoolT> pool() const noexcept
        {
            return m_pool;
        }

        /**
         * @brief Deallocate block
         * @param p Pointer to underlying memory buffer
         */
        static inline void deallocateBlock(void *p)
        {
            auto rawBlock=reinterpret_cast<memorypool::RawBlock*>(reinterpret_cast<uintptr_t>(p)-sizeof(memorypool::RawBlock));
            PoolT::destroy(rawBlock);
        }

    protected:

        /**
         * @brief Allocate buffer
         * @param bytes Buffer size
         * @param alignment ALignment is not used here
         * @return Pointer to allocated buffer
         *
         */
        virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;

        /**
         * @brief Deallocate buffer
         * @param p Buffer to deallocate
         * @param bytes Buffer size is not used here because we use information from the buffer header itself
         * @param alignment Alignment is not used here
         */
        virtual void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override;

        /**
         * @brief Check if two resources are equal
         * @param other Other resource
         * @return True if the same memory pool is used
         */
        virtual bool do_is_equal(const memory_resource& other) const noexcept override
        {
            auto otherPool=dynamic_cast<const SinglePoolMemoryResource*>(&other);
            return otherPool && m_pool.get()==otherPool->m_pool.get();
        }

    private:

        std::shared_ptr<PoolT> m_pool;
};

//---------------------------------------------------------------
template <typename PoolT>
void* SinglePoolMemoryResource<PoolT>::do_allocate(
        std::size_t bytes,
        std::size_t alignment
    )
{
    AssertThrow(bytes<=m_pool->chunkSize(),
            fmt::format("Too large block requested {}>{} in SinglePoolMemoryResource",bytes,m_pool->chunkSize())
        );

    AssertThrow(alignment<=alignof(memorypool::RawBlock),
            fmt::format("Unsupported alignment {}>{} in PoolMemoryResource: sizeof(void*)={}, sizeof(memorypool::RawBlock)={}",
                        alignment,
                        alignof(memorypool::RawBlock),
                        sizeof(void*),
                        sizeof(memorypool::RawBlock)
                )
        );

    return m_pool->allocateRawBlock()->data();
}

//---------------------------------------------------------------
template <typename PoolT>
void SinglePoolMemoryResource<PoolT>::do_deallocate(
        void *p,
        std::size_t bytes,
        std::size_t alignment
    )
{
    std::ignore=bytes;
    std::ignore=alignment;
    deallocateBlock(p);
}

//---------------------------------------------------------------
} // namespace pmr
HATN_COMMON_NAMESPACE_END
#endif // HATNSINGLEPOOLMEMORYRESOURCE_H
