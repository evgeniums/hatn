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
#include <boost/leaf.hpp>

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
    static boost::leaf::result<T> f(const config_tree::HolderT&) noexcept
    {
        return boost::leaf::new_error(makeError(ErrorCode::INVALID_TYPE));
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
    static type f(const config_tree::HolderT& holder) noexcept
    {
        return ValueExtractor<type,Type::String>::f(holder);
    }
};

//---------------------------------------------------------------

template <Type TypeId> struct ValueSetterForward
{
    constexpr static auto id=TypeId;

    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        holder.emplace(std::forward(value));
        typeId=id;
    }
};

template <Type TypeId> struct ValueSetterCast
{
    constexpr static auto id=TypeId;

    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        auto val=static_cast<Storage<id>>(value);
        ValueSetterForward<id>::set(holder,typeId,std::move(val));
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
template <typename T> struct ValueSetter<T,std::enable_if_t<std::is_constructible<std::string,T>::value>> : public ValueSetterForward<Type::String>{};

//---------------------------------------------------------------

} // namespace config_tree_detail

//---------------------------------------------------------------

template <typename T>
boost::leaf::result<T> ConfigTreeValue::as() const noexcept
{
    if (static_cast<bool>(m_value))
    {
        return config_tree_detail::valuesAs<T>::f(m_value);
    }

    if (static_cast<bool>(m_defaultValue))
    {
        return config_tree_detail::valuesAs<T>::f(m_defaultValue);
    }

    return boost::leaf::new_error(makeError(ErrorCode::VALUE_NOT_SET));
}

template <typename T>
const T& ConfigTreeValue::as(common::Error& ec) const noexcept
{
    auto r = as<T>();
    if (!r)
    {
        static T v;
        ec=r.error();
        return v;
    }
    return r.value();
}

template <typename T>
const T& ConfigTreeValue::asThrows() const
{
    auto r = as<T>();
    if (!r)
    {
        throw common::ErrorException{r.error()};
    }
    return r.value();
}

template <typename T>
boost::leaf::result<T> ConfigTreeValue::getDefault() const noexcept
{
    if (static_cast<bool>(m_defaultValue))
    {
        return config_tree_detail::valuesAs<T>::f(m_defaultValue);
    }
    return boost::leaf::new_error(makeError(ErrorCode::VALUE_NOT_SET));
}

template <typename T>
const T& ConfigTreeValue::getDefault(common::Error& ec) const noexcept
{
    auto r = getDefault<T>();
    if (!r)
    {
        static T v;
        ec=r.error();
        return v;
    }
    return r.value();
}

template <typename T>
const T& ConfigTreeValue::getDefaultThrows() const
{
    auto r = getDefault<T>();
    if (!r)
    {
        throw common::ErrorException{r.error()};
    }
    return r.value();
}


//---------------------------------------------------------------

template <typename T>
void ConfigTreeValue::set(T value)
{
    config_tree_detail::ValueSetter<std::decay_t<T>>::set(m_value,m_type,std::forward(value));
}

template <typename T>
void ConfigTreeValue::setDefault(T value)
{
    config_tree_detail::ValueSetter<std::decay_t<T>>::set(m_defaultValue,m_defaultType,std::forward(value));
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_IPP
