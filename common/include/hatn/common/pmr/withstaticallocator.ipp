/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.


  */

/****************************************************************************/
/*

*/
/** @file common/pmr/withstaticallocator.ipp
  *
  *  Implementation of class template for objects that can keep static allocator for all objects of self type
  *
  */

/****************************************************************************/

#ifndef HATNWITHSTATICALLOCATOR_IPP
#define HATNWITHSTATICALLOCATOR_IPP

#define HATN_WITH_STATIC_ALLOCATOR_IMPL \
    template <typename T> \
    hatn::common::pmr::memory_resource* WithStaticAllocator<T>::m_resource=nullptr; \
    template <typename T> \
    void WithStaticAllocator<T>::setStaticResource( \
            hatn::common::pmr::memory_resource* resource \
        ) noexcept \
    { \
        m_resource=resource; \
    } \
    template <typename T> \
    hatn::common::pmr::memory_resource* WithStaticAllocator<T>::getStaticResource() noexcept \
    { \
        return m_resource; \
    }

HATN_COMMON_NAMESPACE_BEGIN
    HATN_WITH_STATIC_ALLOCATOR_IMPL
HATN_COMMON_NAMESPACE_END

#endif // HATNWITHSTATICALLOCATOR_IPP
