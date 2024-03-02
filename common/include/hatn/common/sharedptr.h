/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/sharedptr.h
  *
  *     Shared pointer for managed object
  *
  */

/****************************************************************************/

#ifndef HATNMANAGESHAREDPTR_H
#define HATNMANAGESHAREDPTR_H

#include <hatn/common/config.h>

#include <hatn/common/pointers/std/sharedptr.h>
#include <hatn/common/pointers/mempool/sharedptr.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifdef HATN_SMARTPOINTERS_STD
    //! Shared pointer
    template <typename T> using SharedPtr=pointers_std::SharedPtr<T>;
    //! Class with sharedFromThis() method
    template <typename T,bool DeriveFromManagedObject=true> using EnableSharedFromThis=pointers_std::EnableSharedFromThis<T,DeriveFromManagedObject>;
    //! Embedded shared pointer
    template <typename T> using EmbeddedSharedPtr=pointers_std::SharedPtr<T>;

#else
    //! Shared pointer
    template <typename T> using SharedPtr=pointers_mempool::SharedPtr<T>;
    //! Class with sharedFromThis() method
    template <typename T,bool DeriveFromManagedObject=true> using EnableSharedFromThis=pointers_mempool::EnableSharedFromThis<T,DeriveFromManagedObject>;
    //! Embedded shared pointer
    template <typename T> using EmbeddedSharedPtr=pointers_mempool::EmbeddedSharedPtr<T>;
#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGESHAREDPTR_H
