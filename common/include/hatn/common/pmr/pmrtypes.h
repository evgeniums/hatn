/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pmr/pmrtypes.h
  *
  *     Typedef for polymorphic allocator types
  *
  */

/****************************************************************************/

#ifndef HATNPMRDEFS_H
#define HATNPMRDEFS_H

#include <sstream>

#include <hatn/common/config.h>

#ifdef HATN_PMR_BOOST

#include <boost/container/pmr/memory_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/pool_options.hpp>
#include <boost/container/pmr/vector.hpp>
#include <boost/container/pmr/string.hpp>
#include <boost/container/pmr/list.hpp>
#include <boost/container/pmr/map.hpp>

#else

#include <memory_resource>

#endif

#include <list>
#include <vector>
#include <map>
#include <string>

#include <hatn/common/common.h>
#include <hatn/common/classuid.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pmr {

#ifdef HATN_PMR_BOOST

template <typename T> using polymorphic_allocator = boost::container::pmr::polymorphic_allocator<T>;
using memory_resource = boost::container::pmr::memory_resource;
using pool_options = boost::container::pmr::pool_options;
inline memory_resource* get_default_resource() noexcept
{return boost::container::pmr::get_default_resource();}

template <typename T> using vector=std::vector<T, polymorphic_allocator<T>>;
using string=std::basic_string<char,std::char_traits<char>,polymorphic_allocator<char>>;
template <typename T> using list=std::list<T,polymorphic_allocator<T>>;
#if defined (__APPLE__) || defined (BUILD_ANDROID)
template <typename Key, typename Value> using map=boost::container::pmr::map<Key,Value>;
#else
template <typename Key, typename Value> using map=std::map<Key,Value,std::less<Key>,polymorphic_allocator<std::pair<Key,Value>>>;
#endif
#else

template <typename T> using polymorphic_allocator = std::pmr::polymorphic_allocator<T>;
using memory_resource = std::pmr::memory_resource;
using pool_options = std::pmr::pool_options;
inline memory_resource* get_default_resource() noexcept
{return std::pmr::get_default_resource();}

template <typename T> using vector=std::pmr::vector<T>;
using string=std::pmr::string;
template <typename T> using list=std::pmr::list<T>;
template <typename Key, typename Value> using map=std::pmr::map<Key,Value>;

#endif

using stringstream=std::basic_stringstream<char,
        std::char_traits<char>,
        polymorphic_allocator<char>>;
using stringFromStream=std::basic_string<char,
        std::char_traits<char>,
        polymorphic_allocator<char>>;

using CStringVector=vector<const char*>;

template <typename T> inline void destroyAt(T* obj,polymorphic_allocator<T>& allocator) noexcept
{
#if !defined(HATN_PMR_BOOST) && defined(_MSC_VER)
#if __cplusplus < 201703L
    obj->~T();
#else
    std::destroy_at(obj);
#endif
    std::ignore=allocator;
#else
    allocator.destroy(obj);
#endif
}

template <typename T, typename ... Args> inline T* allocateConstruct(const polymorphic_allocator<T>& allocator,Args&&... args)
{
    auto& alloc=const_cast<polymorphic_allocator<T>&>(allocator);
    auto ptr=alloc.allocate(1);
    alloc.construct(ptr,std::forward<Args>(args)...);
    return ptr;
}

template <typename T> inline void destroyDeallocate(T* obj,polymorphic_allocator<T>& allocator) noexcept
{
    destroyAt(obj,allocator);
    allocator.deallocate(obj,1);
}

//---------------------------------------------------------------
} // namespace memory

HATN_COMMON_NAMESPACE_END
#endif // HATNPMRDEFS_H
