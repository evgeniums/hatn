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

#include <utility>
#include <memory>
#include <type_traits>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Wrapper of pointer with initializer.
template <typename T, typename=void>
struct PointerWithInit
{
    using ElementType=typename std::pointer_traits<T>::element_type;

    ElementType* ptr;
    PointerWithInit(ElementType* ptr=nullptr) noexcept : ptr(ptr)
    {}

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline auto operator ->() noexcept
    {
        return ptr;
    }
    inline auto operator ->() const noexcept
    {
        return ptr;
    }
    inline auto& operator *() const noexcept
    {
        return *ptr;
    }
};

template <typename T>
struct PointerWithInit<
        T,
        std::enable_if_t<!std::is_pointer<T>::value>
    >
{
    T ptr;

    PointerWithInit()
    {}

    PointerWithInit(T ptr) noexcept : ptr(std::move(ptr))
    {}

    bool isNull() const noexcept
    {
        return ptr.isNull();
    }

    inline auto operator ->() noexcept
    {
        return ptr.get();
    }
    inline auto operator ->() const noexcept
    {
        return ptr.get();
    }
    inline auto& operator *() const noexcept
    {
        return *ptr;
    }
};

//! Wrapper of const pointer with initializer.
template <typename T, const std::remove_const_t<typename std::pointer_traits<T>::element_type*> defaultVal=nullptr>
struct ConstPointerWithInit
{    
    using ElementType=std::remove_const_t<typename std::pointer_traits<T>::element_type>;
    const ElementType* ptr;

    ConstPointerWithInit(const ElementType* ptr=defaultVal) noexcept : ptr(ptr)
    {}

    ConstPointerWithInit& operator = (const ElementType* other)
    {
        ptr=other;
        return *this;
    }

    bool isNull() const noexcept
    {
        return ptr==nullptr;
    }

    inline auto operator ->() noexcept
    {
        return ptr;
    }
    inline auto operator ->() const noexcept
    {
        return ptr;
    }
    inline const auto& operator *() const noexcept
    {
        return *ptr;
    }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNPONITERWITHINIT_H
