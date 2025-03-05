/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointers/std/makeshared.h
  *
  *     Helpers to wrap object in shared pointer
  *
  */

/****************************************************************************/

#ifndef HATNMAKESHARED_STD_H
#define HATNMAKESHARED_STD_H

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pointers/std/weakptr.h>
#include <hatn/common/pointers/std/sharedptr.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pointers_std {

struct Pointers
{
    //! Managed object
    using ManagedObject=pointers_std::ManagedObject;

    template <typename T> using EnableManaged=pointers_std::EnableManaged<T>;
    template <typename T> using ManagedWrapper=pointers_std::ManagedWrapper<T>;

    //! Shared pointer
    template <typename T> using SharedPtr=pointers_std::SharedPtr<T>;

    //! Weak pointer
    template <typename T> using WeakPtr=pointers_std::WeakPtr<T>;

    //! Class with sharedFromThis() method
    template <typename T> using EnableSharedFromThis=pointers_std::EnableSharedFromThis<T>;

    //! Make shared object
    template <typename T,typename ... Args> inline static SharedPtr<T> makeShared(Args&&... args)
    {
        return SharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
    }
    //! Allocate shared object from pool
    template <typename T,typename ... Args> inline static SharedPtr<T> allocateShared(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        return SharedPtr<T>(std::allocate_shared<T>(allocator,std::forward<Args>(args)...));
    }
};

//---------------------------------------------------------------
        } // namespace pointers_std
HATN_COMMON_NAMESPACE_END
#endif // HATNMAKESHARED_H
