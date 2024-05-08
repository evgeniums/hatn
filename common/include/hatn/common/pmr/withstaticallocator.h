/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pmr/withstaticallocator.h
  *
  *  Base class for objects that can keep static allocator for all objects of self type
  *
  */

/****************************************************************************/

#ifndef HATNWITHSTATICALLOCATOR_H
#define HATNWITHSTATICALLOCATOR_H

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pmr {

//! Base class for objects that can keep static allocator for all objects of self type
class WithStaticAllocatorBase
{};

} // namespace pmr

//! Base class template for objects that can keep static allocator for all objects of self type
template <typename T>
class WithStaticAllocator : public hatn::common::pmr::WithStaticAllocatorBase
{
    public:

        static void setStaticResource(
            hatn::common::pmr::memory_resource* resource
            ) noexcept;

        static hatn::common::pmr::memory_resource* getStaticResource() noexcept;

    private:

        static hatn::common::pmr::memory_resource* m_resource;
};

HATN_COMMON_NAMESPACE_END

#define HATN_WITH_STATIC_ALLOCATOR_DECLARE(Class,ExportAttr) \
    class Class; \
    template class ExportAttr WithStaticAllocator<Class>;

#endif // HATNWITHSTATICALLOCATOR_H
