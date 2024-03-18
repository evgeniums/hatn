/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreevalue.h
  *
  * Contains declaration of configuration tree node.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREEVALUE_H
#define HATNCONFIGTREEVALUE_H

#include <memory>
#include <string>
#include <vector>
#include <map>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

class ConfigTree;

namespace config_tree {

using MapT = std::map<std::string,std::shared_ptr<ConfigTree>>;

enum class Type : int
{
    None,
    Int,
    Double,
    Bool,
    String,
    ArrayInt,
    ArrayDouble,
    ArrayBool,
    ArrayString,
    ArrayTree,
    Map
};

template <Type TypeId> struct Storage
{using type=int64_t;};

template <> struct Storage<Type::Int>
{using type=int64_t;};

template <> struct Storage<Type::Double>
{using type=double;};

template <> struct Storage<Type::Bool>
{using type=bool;};

template <> struct Storage<Type::String>
{using type=std::string;};

template <> struct Storage<Type::ArrayInt>
{using type=std::vector<int64_t>;};

template <> struct Storage<Type::ArrayDouble>
{using type=std::vector<double>;};

template <> struct Storage<Type::ArrayBool>
{using type=std::vector<bool>;};

template <> struct Storage<Type::ArrayString>
{using type=std::vector<std::string>;};

template <> struct Storage<Type::ArrayTree>
{using type=std::vector<std::shared_ptr<ConfigTree>>;};

template <> struct Storage<Type::Map>
{using type=MapT;};


using ValueT = common::lib::variant<
    Storage<Type::Int>::type,
    Storage<Type::Double>::type,
    Storage<Type::Bool>::type,
    Storage<Type::String>::type,
    Storage<Type::ArrayInt>::type,
    Storage<Type::ArrayDouble>::type,
    Storage<Type::ArrayBool>::type,
    Storage<Type::ArrayString>::type,
    Storage<Type::ArrayTree>::type,
    Storage<Type::Map>::type
    >;

using HolderT=common::lib::optional<config_tree::ValueT>;

} // namespace config_tree

//! Basic element of configuration tree.
class HATN_BASE_EXPORT ConfigTreeValue
{
    public:

        using Type=config_tree::Type;

        ConfigTreeValue()
            :m_type(Type::None),
             m_defaultType(Type::None)
        {}

        ConfigTreeValue(ConfigTreeValue&&)=default;
        ConfigTreeValue& operator =(ConfigTreeValue&&)=default;
        ~ConfigTreeValue()=default;

        ConfigTreeValue(const ConfigTreeValue&)=delete;
        ConfigTreeValue& operator =(const ConfigTreeValue&)=delete;

        Type type(bool orDefault=false) const noexcept
        {
            auto isSet=static_cast<bool>(m_value);
            if (!isSet && orDefault)
            {
                return m_defaultType;
            }
            return m_type;
        }

        Type defaultType() const noexcept
        {
            return m_defaultType;
        }

        bool isSet(bool orDefault=false) const noexcept
        {
            auto ok=static_cast<bool>(m_value);
            if (!ok && orDefault)
            {
                ok=static_cast<bool>(m_defaultValue);
            }
            return ok;
        }

        bool isDefaultSet() const noexcept
        {
            return static_cast<bool>(m_defaultValue);
        }

        void reset() noexcept
        {
            m_type=Type::None;
            m_value.reset();
        }

        void resetDefault() noexcept
        {
            m_defaultType=Type::None;
            m_defaultValue.reset();
        }

        template <typename T> void set(T value);
        template <typename T> void setDefault(T value);

        template <typename T> auto asThrows() const -> decltype(auto);
        template <typename T> auto as(common::Error& ec) const noexcept -> decltype(auto);
        template <typename T> auto as() const noexcept -> decltype(auto);

        template <typename T> auto getDefaultThrows() const -> decltype(auto);
        template <typename T> auto getDefault(common::Error& ec) const noexcept -> decltype(auto);
        template <typename T> auto getDefault() const noexcept -> decltype(auto);

        void toMap()
        {
            m_type=Type::Map;
            m_value.reset();
            m_value.emplace(config_tree::MapT{});
        }

        Result<const config_tree::MapT&> asMap() const noexcept
        {
            if (!static_cast<bool>(m_value))
            {
                return baseErrorResult(BaseError::VALUE_NOT_SET);
            }
            if (m_type!=Type::Map)
            {
                return baseErrorResult(BaseError::INVALID_TYPE);
            }

            return common::lib::variantGet<config_tree::MapT>(m_value.value());
        }

        Result<config_tree::MapT&> asMap() noexcept
        {
            if (!static_cast<bool>(m_value))
            {
                return baseErrorResult(BaseError::VALUE_NOT_SET);
            }
            if (m_type!=Type::Map)
            {
                return baseErrorResult(BaseError::INVALID_TYPE);
            }

            return common::lib::variantGet<config_tree::MapT>(m_value.value());
        }

        const config_tree::MapT& asMap(common::Error& ec) const noexcept
        {
            auto&& r = asMap();
            if (!r)
            {
                static config_tree::MapT v;
                ec=r.error();
                return v;
            }
            return r.takeValue();
        }

        config_tree::MapT& asMap(common::Error& ec) noexcept
        {
            auto&& r = asMap();
            if (!r)
            {
                static config_tree::MapT v;
                ec=r.error();
                return v;
            }
            return r.takeValue();
        }

        const config_tree::MapT& asMapThrows() const
        {
            auto&& r = asMap();
            if (!r)
            {
                throw common::ErrorException{r.error()};
            }
            return r.takeValue();
        }

        config_tree::MapT& asMapThrows()
        {
            auto&& r = asMap();
            if (!r)
            {
                throw common::ErrorException{r.error()};
            }
            return r.takeValue();
        }

    private:

        Type m_type;
        Type m_defaultType;
        config_tree::HolderT m_value;
        config_tree::HolderT m_defaultValue;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEVALUE_H
