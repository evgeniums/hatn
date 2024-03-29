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

WeakPool* WeakPool::m_instance=nullptr;
size_t WeakPool::DefaultObjectsCount=4096;

//---------------------------------------------------------------
WeakPool* WeakPool::init(
        pmr::memory_resource *memResource,
        bool ownMemResource
    )
{
    m_instance=new WeakPool(memResource,ownMemResource);
    return m_instance;
}

//---------------------------------------------------------------
void WeakPool::free() noexcept
{
    delete m_instance;
    m_instance=nullptr;
}

//---------------------------------------------------------------
WeakPool::WeakPool(
        pmr::memory_resource *memResource,
        bool ownMemResource
    ) : m_ownedMemResource(ownMemResource?memResource:nullptr),
        m_allocator(memResource)
{
}

//---------------------------------------------------------------
        } // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
