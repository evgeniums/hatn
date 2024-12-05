/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointers/mempool/makeshared.h
  *
  *     Helpers to wrap object in shared pointer
  *
  */

/****************************************************************************/

#ifndef HATNMAKESHARED_MP_H
#define HATNMAKESHARED_MP_H

#include <functional>

#include <hatn/common/common.h>

#include <hatn/common/pointers/mempool/managedobject.h>
#include <hatn/common/pointers/mempool/weakptr.h>
#include <hatn/common/pointers/mempool/sharedptr.h>

#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

template <typename T, typename=void> struct SharedCreator
{
};
template <typename T> struct SharedCreator<T,std::enable_if_t<std::is_base_of<ManagedObject,T>::value>>
{
    template <typename ... Args>
    inline static SharedPtr<T> allocate(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        auto ptr=pmr::allocateConstruct(allocator,std::forward<Args>(args)...);
        return SharedPtr<T>(ptr,ptr,allocator.resource());
    }

    template <typename ... Args>
    inline static SharedPtr<T> make(Args&&... args)
    {
        auto allocator=pmr::polymorphic_allocator<T>(pmr::get_default_resource());
        return allocate(allocator,std::forward<Args>(args)...);
    }
};
template <typename T> struct SharedCreator<T,std::enable_if_t<!std::is_base_of<ManagedObject,T>::value>>
{
    template <typename ... Args>
    inline static SharedPtr<T> allocate(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        auto ptr=SharedCreator<ManagedWrapper<T>>::allocate(allocator,std::forward<Args>(args)...);
        return ptr.template staticCast<T>();
    }

    template <typename ... Args>
    inline static SharedPtr<T> make(Args&&... args)
    {
        auto ptr=SharedCreator<ManagedWrapper<T>>::make(std::forward<Args>(args)...);
        return ptr.template staticCast<T>();
    }
};

struct Pointers
{
    //! Managed object
    using ManagedObject=pointers_mempool::ManagedObject;

    template <typename T> using EnableManaged=pointers_mempool::EnableManaged<T>;
    template <typename T> using ManagedWrapper=pointers_mempool::ManagedWrapper<T>;

    //! Shared pointer
    template <typename T> using SharedPtr=pointers_mempool::SharedPtr<T>;

    //! Weak pointer
    template <typename T> using WeakPtr=pointers_mempool::WeakPtr<T>;

    //! Class with sharedFromThis() method
    template <typename T> using EnableSharedFromThis=pointers_mempool::EnableSharedFromThis<T>;

    //! Make shared object
    template <typename T,typename ... Args>
    inline static SharedPtr<T> makeShared(Args&&... args)
    {
        return SharedCreator<T>::make(std::forward<Args>(args)...);
    }

    //! Allocate shared object using allocator, const for use with temporary created allocated
    template <typename T,typename ... Args>
    inline static SharedPtr<T> allocateShared(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        return SharedCreator<T>::allocate(allocator,std::forward<Args>(args)...);
    }
};

//---------------------------------------------------------------
        } // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
#endif // HATNMAKESHARED_H
