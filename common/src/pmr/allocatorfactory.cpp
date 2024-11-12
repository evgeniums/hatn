/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pmr/allocatorfactory.сpp
  *
  *      Factory of allocators for allocation of objects and data fields
  *
  */

#include <hatn/common/pmr/allocatorfactory.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pmr {

std::shared_ptr<AllocatorFactory>& defaultfactory()
{
    static std::shared_ptr<AllocatorFactory> inst=std::make_shared<AllocatorFactory>();
    return inst;
}

//---------------------------------------------------------------
//! @todo Use const AllocatorFactory*
AllocatorFactory* AllocatorFactory::getDefault() noexcept
{
    return defaultfactory().get();
}

//---------------------------------------------------------------
void AllocatorFactory::setDefault(std::shared_ptr<AllocatorFactory> instance) noexcept
{
    defaultfactory()=std::move(instance);
}

//---------------------------------------------------------------
void AllocatorFactory::resetDefault()
{
    defaultfactory()=std::make_shared<AllocatorFactory>();
}

//---------------------------------------------------------------
AllocatorFactory::AllocatorFactory(
    ) : AllocatorFactory(
                common::pmr::get_default_resource(),
                common::pmr::get_default_resource()
            )
{}

//---------------------------------------------------------------
AllocatorFactory::AllocatorFactory(
        common::pmr::memory_resource *objectMemoryResource,
        common::pmr::memory_resource *dataMemoryResource,
        bool ownObjectResource,
        bool ownDataResource
    ) : m_objectResourceRef(objectMemoryResource),
        m_dataResourceRef(dataMemoryResource)
{
    if (ownObjectResource)
    {
        m_objectResource.reset(objectMemoryResource);
    }
    if (ownDataResource)
    {
        m_objectResource.reset(dataMemoryResource);
    }    
}

//---------------------------------------------------------------
        } // namespace pmr
HATN_COMMON_NAMESPACE_END
