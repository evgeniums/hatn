/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/pmr/poolmemoryresource.h
  *
  *     Polymorphic memory resource using memory pools
  *
  */

/****************************************************************************/

#ifndef HATNPOOLMEMORYRESOURCE_H
#define HATNPOOLMEMORYRESOURCE_H

#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/memorypool/pool.h>
#include <hatn/common/memorypool/bucket.h>
#include <hatn/common/memorypool/poolcachegen.h>
#include <hatn/common/locker.h>

#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pmr {

/**
 * @brief Memory resource that uses pools as backend
 *
 * Memory pool will be created on the first allocation request if createEmbeddedPool is set.
 * If there is no embedded pool and createEmbeddedPool is not set
 * or if the requested size does not fit into the chunk of embedded pool
 * then some other suitable pool from poolCacheGen will be used.
 * If requested size is more than poolCacheGen.maxBlockSize then the buffer will be allocated with ::operator new()
 *
 */
template <typename PoolT>
class PoolMemoryResource final : public memory_resource
{
    public:

        /**
         * @brief Options of the memory pool resource
         */
        struct Options final
        {
            //! Ctor
            explicit Options(
                std::shared_ptr<memorypool::PoolCacheGen<PoolT>> poolCacheGen=std::shared_ptr<memorypool::PoolCacheGen<PoolT>>(), //!< Cache and generator of memory pools
                bool createEmbeddedPool=true //!< Create embedded pool on the first allocation request
            ) noexcept : poolCacheGen(std::move(poolCacheGen)),
                         createEmbeddedPool(createEmbeddedPool)
            {}

            /**
             * @brief Cache and generator of memory pools
             */
            std::shared_ptr<memorypool::PoolCacheGen<PoolT>> poolCacheGen;

            /**
             * @brief Create embedded pool on the first allocation request
             */
            bool createEmbeddedPool;
        };

        /**
         * @brief Ctor from existing memory pool
         * @param pool Memory pool to use as backend for this resource
         */
        explicit PoolMemoryResource(
            std::shared_ptr<PoolT> pool
        ) noexcept :
            m_pool(std::move(pool)),
            m_poolPtr(m_pool.get())
        {}

        /**
         * @brief Ctor from options
         * @param opts Pool options
         */
        explicit PoolMemoryResource(
            Options opts
        ) noexcept : m_poolOptions(std::move(opts)),
            m_poolPtr(nullptr)
        {}

        /**
         * @brief Ctor from pool cache generator
         * @param poolCacheGen Pool cache generator
         */
        explicit PoolMemoryResource(
                std::shared_ptr<memorypool::PoolCacheGen<PoolT>> poolCacheGen
            ) noexcept : PoolMemoryResource(Options(std::move(poolCacheGen)))
        {}

        //! Default ctor
        PoolMemoryResource(
            ) : PoolMemoryResource(std::make_shared<memorypool::PoolCacheGen<PoolT>>())
        {}

        /**
         * @brief Get memory pool
         * @return Underlying memory pool
         */
        inline std::shared_ptr<PoolT> pool() const noexcept
        {
            return m_pool;
        }

        /**
         * @brief Get pool options
         * @return Copy of pool options
         */
        inline Options options() const noexcept
        {
            return m_poolOptions;
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
         * @return True if the this resource
         */
        virtual bool do_is_equal(const memory_resource& other) const noexcept override
        {
            return &other==this;
        }

    private:

        Options m_poolOptions;
        std::shared_ptr<PoolT> m_pool;
        std::atomic<PoolT*> m_poolPtr;

        inline void* allocateWhenEmbeddedPool(std::size_t bytes);
        inline std::shared_ptr<PoolT> poolFromCaheGen(std::size_t bytes);
};

inline void* allocateWithOperatorNew(size_t bytes)
{
    auto block=static_cast<memorypool::RawBlock*>(
                ::operator new(bytes+sizeof(memorypool::RawBlock)
                           #if __cplusplus >= 201703L
                               , static_cast<std::align_val_t>(alignof(memorypool::RawBlock))
                           #endif
                               ));
    block->resetBucket();
    return block->data();
}

//---------------------------------------------------------------
template <typename PoolT>
inline std::shared_ptr<PoolT> PoolMemoryResource<PoolT>::poolFromCaheGen(
        std::size_t bytes
    )
{
    AssertThrow(m_poolOptions.poolCacheGen,"Pool cache gen is not set in PoolMemoryResource");
    auto pool=m_poolOptions.poolCacheGen->pool(bytes);
    AssertThrow(pool,fmt::format("No pool of suitable size {} found in PoolMemoryResource",bytes));
    return pool;
}

//---------------------------------------------------------------
template <typename PoolT>
inline void* PoolMemoryResource<PoolT>::allocateWhenEmbeddedPool(
        std::size_t bytes
    )
{
    auto poolPtr=m_poolPtr.load(std::memory_order_relaxed);
    if (bytes<=poolPtr->chunkSize())
    {
        return poolPtr->allocateRawBlock()->data();
    }

    AssertThrow(m_poolOptions.poolCacheGen,"Pool cache gen is not set in PoolMemoryResource");
    if (
        bytes>m_poolOptions.poolCacheGen->sizes().maxSizeOfBlock()
                         &&
        m_poolOptions.poolCacheGen->sizes().maxSizeOfBlock()!=0
       )
    {
        return allocateWithOperatorNew(bytes);
    }

    return poolFromCaheGen(bytes)->allocateRawBlock()->data();
}

//---------------------------------------------------------------
template <typename PoolT>
void* PoolMemoryResource<PoolT>::do_allocate(
        std::size_t bytes,
        std::size_t alignment
    )
{
    AssertThrow(alignment<=alignof(memorypool::RawBlock),
            fmt::format("Unsupported alignment {}>{} in PoolMemoryResource: sizeof(void*)={}, sizeof(memorypool::RawBlock)={}",
                        alignment,
                        alignof(memorypool::RawBlock),
                        sizeof(void*),
                        sizeof(memorypool::RawBlock)
                        )
           )
    if (m_poolPtr.load(std::memory_order_acquire)!=nullptr)
    {
        return allocateWhenEmbeddedPool(bytes);
    }

    if (m_poolOptions.createEmbeddedPool)
    {
        auto pool=poolFromCaheGen(bytes);

        PoolT* tmp=nullptr;
        if (m_poolPtr.compare_exchange_strong(tmp,pool.get(),std::memory_order_seq_cst))
        {
            m_pool=std::move(pool);
        }

        return allocateWhenEmbeddedPool(bytes);
    }

    return poolFromCaheGen(bytes)->allocateRawBlock()->data();
}

//---------------------------------------------------------------
template <typename PoolT>
void PoolMemoryResource<PoolT>::do_deallocate(
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
#endif // HATNPOOLMEMORYRESOURCE_H
