/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/makeshared.h
  *
  *     Helpers to wrap object in shared pointer
  *
  */

/****************************************************************************/

#ifndef HATNMAKESHARED_H
#define HATNMAKESHARED_H

#include <hatn/common/config.h>

#include <hatn/common/sharedptr.h>

#include <hatn/common/pointers/std/makeshared.h>
#include <hatn/common/pointers/mempool/makeshared.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifdef HATN_SMARTPOINTERS_STD

    //! Make shared object
    template <typename T,typename ... Args> SharedPtr<T> inline makeShared(Args&&... args)
    {
        return pointers_std::Pointers::makeShared<T>(std::forward<Args>(args)...);
    }
    //! Allocate shared object using allocator
    template <typename T,typename ... Args> SharedPtr<T> inline allocateShared(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        return pointers_std::Pointers::allocateShared(allocator,std::forward<Args>(args)...);
    }

#else

    //! Make shared object
    template <typename T,typename ... Args> inline SharedPtr<T> makeShared(Args&&... args)
    {
        return pointers_mempool::Pointers::makeShared<T>(std::forward<Args>(args)...);
    }
    //! Allocate shared object using allocator, const for use with temporary created allocated
    template <typename T,typename ... Args> SharedPtr<T> inline allocateShared(const pmr::polymorphic_allocator<T>& allocator,Args&&... args)
    {
        return pointers_mempool::Pointers::allocateShared(allocator,std::forward<Args>(args)...);
    }

#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNMAKESHARED_H
