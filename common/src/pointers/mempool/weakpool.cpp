/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/pointers/mempool/weakptr.сpp
  *
  *     Singleton memory pool for control structures of weak pointers
  *
  */

#include <hatn/common/pointers/mempool/weakpool.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

/********************** WeakPool **************************/

HATN_SINGLETON_INIT(WeakPool)

size_t WeakPool::DefaultObjectsCount=4096;

//---------------------------------------------------------------

WeakPool& WeakPool::instance()
{
    static WeakPool m_instance;
    return m_instance;
}

//---------------------------------------------------------------
WeakPool& WeakPool::init(
        pmr::memory_resource *memResource,
        bool ownMemResource
    )
{
    instance().m_ownedMemResource=ownMemResource?memResource:nullptr;
    if (!instance().m_allocator)
    {
        instance().m_allocator.reset(new pmr::polymorphic_allocator<WeakCtrl>{memResource});
    }
    return instance();
}

//---------------------------------------------------------------
void WeakPool::free() noexcept
{
    if (instance().m_ownedMemResource!=nullptr)
    {
        instance().m_allocator.reset();
        delete instance().m_ownedMemResource;
    }
    instance().m_ownedMemResource=nullptr;
}

//---------------------------------------------------------------

} // namespace pointers_mempool

HATN_COMMON_NAMESPACE_END
