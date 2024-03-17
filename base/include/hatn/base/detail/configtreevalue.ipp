/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtree.h
  *
  * Contains declarations of configuration tree types and functions.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREE_IPP
#define HATNCONFIGTREE_IPP

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>
#include <hatn/base/configtreevalue.h>

HATN_BASE_NAMESPACE_BEGIN

namespace config_tree_detail {

using Type=config_tree::Type;
template <Type TypeId> using Storage=config_tree::Storage<TypeId>;
using HolderT=config_tree::HolderT;

//---------------------------------------------------------------

//! Extract requested type from config tree value.
template <typename T, Type TypeId>
struct ValueExtractor
{
    using storageType=typename Storage<TypeId>::type;

    static T f(const config_tree::HolderT& holder) noexcept
    {
        return static_cast<T>(common::lib::variantGet<storageType>(holder.value()));
    }
};

//---------------------------------------------------------------

//! Helpers for extracting concrete types from config tree value holder.
template <typename T> struct valuesAs
{
    static Result<T> f(const config_tree::HolderT&) noexcept
    {
        return errorResult(ErrorCode::INVALID_TYPE);
    }
};

template <> struct valuesAs<int64_t>
{
    using type=int64_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<uint64_t>
{
    using type=uint64_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<int32_t>
{
    using type=int32_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<uint32_t>
{
    using type=uint32_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<int16_t>
{
    using type=int16_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<uint16_t>
{
    using type=uint16_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<int8_t>
{
    using type=int8_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<uint8_t>
{
    using type=uint8_t;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Int>::f(holder);
    }
};

template <> struct valuesAs<double>
{
    using type=double;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Double>::f(holder);
    }
};

template <> struct valuesAs<float>
{
    using type=float;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Double>::f(holder);
    }
};

template <> struct valuesAs<bool>
{
    using type=bool;
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::Bool>::f(holder);
    }
};

template <> struct valuesAs<std::string>
{
    using type=std::string;
    using storageType=typename Storage<Type::String>::type;

    static const type& f(const config_tree::HolderT& holder) noexcept
    {
        return common::lib::variantGet<storageType>(holder.value());
    }
};

//---------------------------------------------------------------

template <Type TypeId> struct ValueSetterMove
{
    constexpr static auto id=TypeId;

    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        holder.emplace(std::move(value));
        typeId=id;
    }
};

template <Type TypeId> struct ValueSetterCast
{
    constexpr static auto id=TypeId;

    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        auto val=static_cast<typename Storage<id>::type>(value);
        ValueSetterMove<id>::set(holder,typeId,std::move(val));
    }
};

template <typename T, typename T1=void> struct ValueSetter
{
    constexpr static auto id=Type::None;

    template <typename ValueT>
    static void set(HolderT&, Type&, ValueT) noexcept {}
};

template <> struct ValueSetter<bool> : public ValueSetterCast<Type::Bool>{};
template <typename T> struct ValueSetter<T,std::enable_if_t<std::is_integral<T>::value>> : public ValueSetterCast<Type::Int>{};
template <typename T> struct ValueSetter<T,std::enable_if_t<std::is_floating_point<T>::value>> : public ValueSetterCast<Type::Double>{};

template <typename T> struct ValueSetter<T,std::enable_if_t<std::is_constructible<std::string,T>::value>> : public ValueSetterMove<Type::String>
{
    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        holder.emplace(std::string{std::move(value)});
        typeId=id;
    }
};

//---------------------------------------------------------------

} // namespace config_tree_detail

//---------------------------------------------------------------

template <typename T>
auto ConfigTreeValue::as() const noexcept -> decltype(auto)
{
    using valueT=decltype(config_tree_detail::valuesAs<T>::f(m_value));
    auto expectedTypeId=config_tree_detail::ValueSetter<std::decay_t<T>>::id;

    if (static_cast<bool>(m_value))
    {
        if (expectedTypeId!=m_type)
        {
            return Result<valueT>{errorResult(ErrorCode::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_value));
    }

    if (static_cast<bool>(m_defaultValue))
    {
        if (expectedTypeId!=m_defaultType)
        {
            return Result<valueT>{errorResult(ErrorCode::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    }

    return Result<valueT>{errorResult(ErrorCode::VALUE_NOT_SET)};
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4172 )
#endif

template <typename T>
const T& ConfigTreeValue::as(common::Error& ec) const noexcept
{
    const auto r = as<T>();
    if (r)
    {
        ec=r.error();
        return r.underlyingValue();
    }
    return r.value();
}

template <typename T>
auto ConfigTreeValue::asThrows() const  -> decltype(auto)
{
    const auto r = as<T>();
    if (r)
    {
        throw common::ErrorException{r.error()};
    }
    return r.value();
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

template <typename T>
auto ConfigTreeValue::getDefault() const noexcept -> decltype(auto)
{
    using valueT=decltype(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    auto expectedTypeId=config_tree_detail::ValueSetter<std::decay_t<T>>::id;
    if (static_cast<bool>(m_defaultValue))
    {
        if (expectedTypeId!=m_defaultType)
        {
            return Result<valueT>{errorResult(ErrorCode::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    }
    return Result<valueT>{errorResult(ErrorCode::VALUE_NOT_SET)};
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4172 )
#endif

template <typename T>
auto ConfigTreeValue::getDefault(common::Error& ec) const noexcept -> decltype(auto)
{
    const auto r = getDefault<T>();
    if (r)
    {
        ec=r.error();
        return r.underlyingValue();
    }
    return r.value();
}

template <typename T>
auto ConfigTreeValue::getDefaultThrows() const -> decltype(auto)
{
    const auto r = getDefault<T>();
    if (r)
    {
        throw common::ErrorException{r.error()};
    }
    return r.value();
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

//---------------------------------------------------------------

template <typename T>
void ConfigTreeValue::set(T value)
{
    config_tree_detail::ValueSetter<std::decay_t<T>>::set(m_value,m_type,std::move(value));
}

template <typename T>
void ConfigTreeValue::setDefault(T value)
{
    config_tree_detail::ValueSetter<std::decay_t<T>>::set(m_defaultValue,m_defaultType,std::move(value));
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_IPP
