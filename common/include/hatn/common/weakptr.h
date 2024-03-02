/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/weakptr.h
  *
  *     Weak pointer for managed objects
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDWEAKPTR_H
#define HATNMANAGEDWEAKPTR_H

#include <hatn/common/config.h>

#include <hatn/common/pointers/mempool/weakptr.h>
#include <hatn/common/pointers/std/weakptr.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifdef HATN_SMARTPOINTERS_STD
    //! Weak pointer
    template <typename T> using WeakPtr=pointers_std::WeakPtr<T>;
#else
    //! Weak pointer
    template <typename T> using WeakPtr=pointers_mempool::WeakPtr<T>;
#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDWEAKPTR_H
