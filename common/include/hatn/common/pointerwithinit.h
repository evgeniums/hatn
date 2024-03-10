/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointerwithinit.h
  *
  *   Wrappers of pointers with initializers.
  *
  */

/****************************************************************************/

#ifndef HATNPONITERWITHINIT_H
#define HATNPONITERWITHINIT_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Wrapper of pointer with initializer.
template <typename T> struct PointerWithInit
{
    T* ptr;
    PointerWithInit(T* ptr=nullptr) noexcept : ptr(ptr)
    {}

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline T* operator ->() noexcept
    {
        return ptr;
    }
    inline T* operator ->() const noexcept
    {
        return ptr;
    }
};

//! Wrapper of const pointer with initializer.
template <typename T, const T* defaultVal=nullptr> struct ConstPointerWithInit
{
    const T* ptr;
    ConstPointerWithInit(const T* ptr=defaultVal) noexcept : ptr(ptr)
    {}

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline const T* operator ->() noexcept
    {
        return ptr;
    }
    inline const T* operator ->() const noexcept
    {
        return ptr;
    }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNPONITERWITHINIT_H
