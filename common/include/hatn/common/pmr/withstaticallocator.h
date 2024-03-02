/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
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
{
};

} // pmr
HATN_COMMON_NAMESPACE_END

//! Base class template for objects that can keep static allocator for all objects of self type
#define HATN_WITH_STATIC_ALLOCATOR_DECL \
    template <typename T> \
    class WithStaticAllocator : public hatn::common::pmr::WithStaticAllocatorBase \
    { \
        public: \
    \
            static void setStaticResource( \
                hatn::common::pmr::memory_resource* resource \
            ) noexcept; \
    \
            static hatn::common::pmr::memory_resource* getStaticResource() noexcept; \
    \
        private: \
    \
            static hatn::common::pmr::memory_resource* m_resource; \
    };

HATN_COMMON_NAMESPACE_BEGIN

HATN_WITH_STATIC_ALLOCATOR_DECL

HATN_COMMON_NAMESPACE_END

#define HATN_WITH_STATIC_ALLOCATOR_INLINE_H \
    HATN_IGNORE_UNUSED_FUNCTION_BEGIN \
    HATN_WITH_STATIC_ALLOCATOR_DECL \
    HATN_IGNORE_UNUSED_FUNCTION_END

#define HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC \
    HATN_IGNORE_UNUSED_FUNCTION_BEGIN \
    HATN_WITH_STATIC_ALLOCATOR_DECL \
    HATN_WITH_STATIC_ALLOCATOR_IMPL \
    HATN_IGNORE_UNUSED_FUNCTION_END

#define HATN_WITH_STATIC_ALLOCATOR_DECLARE(Name,ExportAttr) \
    class Name; \
    template class ExportAttr WithStaticAllocator<Name>;

#endif // HATNWITHSTATICALLOCATOR_H
