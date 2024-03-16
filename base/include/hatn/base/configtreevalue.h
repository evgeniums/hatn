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
#include <boost/leaf.hpp>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

class ConfigTree;

namespace config_tree {

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
    MapInt,
    MapDouble,
    MapBool,
    MapString,
    MapTree
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

template <> struct Storage<Type::MapInt>
{using type=std::map<std::string,int64_t>;};

template <> struct Storage<Type::MapDouble>
{using type=std::map<std::string,double>;};

template <> struct Storage<Type::MapBool>
{using type=std::map<std::string,bool>;};

template <> struct Storage<Type::MapString>
{using type=std::map<std::string,std::string>;};

template <> struct Storage<Type::MapTree>
{using type=std::map<std::string,std::shared_ptr<ConfigTree>>;};


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
    Storage<Type::MapInt>::type,
    Storage<Type::MapDouble>::type,
    Storage<Type::MapBool>::type,
    Storage<Type::MapString>::type,
    Storage<Type::MapTree>::type
    >;

using HolderT=common::lib::optional<config_tree::ValueT>;

} // namespace config_tree

//! Basic element of configuration tree.
class HATN_BASE_EXPORT ConfigTreeValue
{
    public:

        using Type=config_tree::Type;

        Type type() const noexcept
        {
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
            m_value.reset();
        }

        template <typename T> void set(T value);
        template <typename T> void setDefault(T value);

        template <typename T> const T& asThrows() const;
        template <typename T> const T& as(common::Error& ec) const noexcept;
        template <typename T> boost::leaf::result<T> as() const noexcept;

        template <typename T> const T& getDefaultThrows() const;
        template <typename T> const T& getDefault(common::Error& ec) const noexcept;
        template <typename T> boost::leaf::result<T> getDefault() const noexcept;

    private:

        Type m_type;
        Type m_defaultType;
        config_tree::HolderT m_value;
        config_tree::HolderT m_defaultValue;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEVALUE_H
