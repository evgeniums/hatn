/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/pointers/mempool/weakpool.h
  *
  *     Singleton memory pool for control structures of weak pointers
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDWEAKPOOL_MP_H
#define HATNMANAGEDWEAKPOOL_MP_H

#include <memory>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/pointers/mempool/weakctrl.h>
#include <hatn/common/pointers/mempool/managedobject.h>
#include <hatn/common/singleton.h>

#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

//! Singleton memory pool for control structures of weak pointers.
class HATN_COMMON_EXPORT WeakPool final : public Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        //! Number of objects in pool's bucket.
        static size_t DefaultObjectsCount; // default count is 4096

        /**
         * @brief Init pool instance.
         * @param memResource Memory resource for polymorphic_allocator.
         * @param WeakPool will own the memory resource and destroy it in destructor.
         * @return Pool instance.
         *
         * @attention Call init() only in the main thread before all other threads start.
         */
        static WeakPool& init(
            pmr::memory_resource* memResource=pmr::get_default_resource(),
            bool ownMemResource=false
        );

        /**
         * @brief Get pool instance.
         * @return Pool instance.
         */
        static WeakPool& instance();

        /**
         * @brief Release pool instance.
         *
         * @attention For thread safety call free() only in the main thread after all other threads have stopped.
         */
        static void free() noexcept;

        //! Create WeakCtrl object.
        inline WeakCtrl* allocate(ManagedObject* obj)
        {
            assert(m_allocator);
            auto* ctrl=m_allocator->allocate(1);
            m_allocator->construct(ctrl,obj);
            auto* actualCtrl=obj->setWeakCtrl(ctrl);
            if (ctrl!=actualCtrl)
            {
                deallocate(ctrl);
                ctrl=actualCtrl;
            }
            return ctrl;
        }

        //! Create WeakCtrl object.
        inline void deallocate(WeakCtrl* ctrl) noexcept
        {
            assert(m_allocator);
            pmr::destroyDeallocate(ctrl,*m_allocator);
        }

        bool isValid() const noexcept
        {
            return static_cast<bool>(m_allocator);
        }

    private:

        WeakPool() : m_ownedMemResource(nullptr)
        {}

        pmr::memory_resource* m_ownedMemResource;
        std::unique_ptr<pmr::polymorphic_allocator<WeakCtrl>> m_allocator;
};

//---------------------------------------------------------------
inline void WeakCtrl::reset() noexcept
{
    if (m_refCount.fetch_sub(1, std::memory_order_acquire) == 1)
    {
        WeakPool::instance().deallocate(this);
    }
}

//---------------------------------------------------------------
        } // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDWEAKPOOL_MP_H
