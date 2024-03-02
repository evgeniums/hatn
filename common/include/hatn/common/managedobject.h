/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/managedobject.h
  *
  *     Base class for managed objects stored in memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDOBJECT_H
#define HATNMANAGEDOBJECT_H

#include <hatn/common/config.h>

#include <hatn/common/pointers/std/managedobject.h>
#include <hatn/common/pointers/mempool/managedobject.h>

HATN_COMMON_NAMESPACE_BEGIN

#ifdef HATN_SMARTPOINTERS_STD

//! Base managed object
using ManagedObject = pointers_std::ManagedObject;

template <typename T> using EnableManaged=pointers_std::EnableManaged<T>;
template <typename T> using ManagedWrapper=pointers_std::ManagedWrapper<T>;
using ManagedObject=pointers_std::ManagedObject;

#else

//! Base managed object
using ManagedObject = pointers_mempool::ManagedObject;

template <typename T> using EnableManaged=pointers_mempool::EnableManaged<T>;
template <typename T> using ManagedWrapper=pointers_mempool::ManagedWrapper<T>;
using ManagedObject=pointers_mempool::ManagedObject;

#endif

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDOBJECT_H
