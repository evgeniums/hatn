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

#include <boost/hana/map.hpp>
#include <boost/hana/fwd/at_key.hpp>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

class ConfigTree;
class ConfigTreePath;

namespace config_tree {

using SubtreeT=std::unique_ptr<ConfigTree>;
using MapT = std::map<std::string,SubtreeT>;

inline SubtreeT makeTree()
{
    return std::make_unique<ConfigTree>();
}

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

enum class NumericType : int
{
    None,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64
};

constexpr auto NumericTypes = hana::make_map(
    hana::make_pair(hana::type<int8_t>{}, NumericType::Int8),
    hana::make_pair(hana::type<int16_t>{}, NumericType::Int16),
    hana::make_pair(hana::type<int32_t>{}, NumericType::Int32),
    hana::make_pair(hana::type<int64_t>{}, NumericType::Int64),
    hana::make_pair(hana::type<uint8_t>{}, NumericType::UInt8),
    hana::make_pair(hana::type<uint16_t>{}, NumericType::UInt16),
    hana::make_pair(hana::type<uint32_t>{}, NumericType::UInt32),
    hana::make_pair(hana::type<uint64_t>{}, NumericType::UInt64)
);

template <Type TypeId> struct Storage
{using type=int64_t;constexpr static bool isArray=false;};

template <> struct Storage<Type::Int>
{using type=int64_t;constexpr static bool isArray=false;};

template <> struct Storage<Type::Double>
{using type=double;constexpr static bool isArray=false;};

template <> struct Storage<Type::Bool>
{using type=bool;constexpr static bool isArray=false;};

template <> struct Storage<Type::String>
{using type=std::string;constexpr static bool isArray=false;};

template <> struct Storage<Type::ArrayInt>
{using type=std::vector<int64_t>;constexpr static bool isArray=true;};

template <> struct Storage<Type::ArrayDouble>
{using type=std::vector<double>;constexpr static bool isArray=true;};

template <> struct Storage<Type::ArrayBool>
{using type=std::vector<bool>;constexpr static bool isArray=true;};

template <> struct Storage<Type::ArrayString>
{using type=std::vector<std::string>;constexpr static bool isArray=true;};

template <> struct Storage<Type::ArrayTree>
{using type=std::vector<SubtreeT>;constexpr static bool isArray=true;};

template <> struct Storage<Type::Map>
{using type=MapT;constexpr static bool isArray=false;};

inline constexpr bool isArray(Type typ) noexcept
{
    return typ==Type::ArrayTree || typ==Type::ArrayString || typ==Type::ArrayInt || typ==Type::ArrayDouble || typ==Type::ArrayBool;
}

inline constexpr bool isScalar(Type typ) noexcept
{
    return typ==Type::Int || typ==Type::String || typ==Type::Double || typ==Type::Bool;
}

inline constexpr bool isMap(Type typ) noexcept
{
    return typ==Type::Map;
}

template <typename T, typename T1=void> struct ValueType
{
    using origin=T;

    constexpr static auto id=Type::None;
    using type=typename Storage<id>::type;

    constexpr static auto arrayId=Type::None;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return NumericType::None;
    }
};

template <typename T> struct ValueType<T,std::enable_if_t<std::is_same<bool,T>::value>>
{
    using origin=T;

    constexpr static auto id=Type::Bool;
    using type=typename Storage<id>::type;

    constexpr static auto arrayId=Type::ArrayBool;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return NumericType::None;
    }
};

template <typename T> struct ValueType<T,std::enable_if_t<!std::is_same<bool,T>::value && std::is_integral<T>::value>>
{
    using origin=T;

    constexpr static auto id=Type::Int;
    using type=typename Storage<id>::type;

    constexpr static auto arrayId=Type::ArrayInt;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return hana::at_key(NumericTypes, hana::type<T>{});
    }
};

template <typename T> struct ValueType<T,std::enable_if_t<std::is_floating_point<T>::value>>
{
    using origin=T;

    constexpr static auto id=Type::Double;
    using type=typename Storage<id>::type;

    constexpr static auto arrayId=Type::ArrayDouble;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return NumericType::None;
    }
};

template <typename T> struct ValueType<T,std::enable_if_t<std::is_constructible<std::string,T>::value>>
{
    using origin=T;

    constexpr static auto id=Type::String;
    using type=typename Storage<id>::type;

    constexpr static auto arrayId=Type::ArrayString;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return NumericType::None;
    }
};

template <typename T> struct ValueType<T,std::enable_if_t<std::is_same<ConfigTree,T>::value>>
{
    using origin=SubtreeT;

    constexpr static auto id=Type::None;
    using type=SubtreeT;

    constexpr static auto arrayId=Type::ArrayTree;
    using arrayType=typename Storage<arrayId>::type;

    constexpr static auto numericId() noexcept
    {
        return NumericType::None;
    }
};

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

enum class ArrayMerge: int
{
    Preserve,
    Merge,
    Append,
    Prepend
};

} // namespace config_tree

/**
 * @brief The ArrayView class to work with array values.
 */
template <typename T, bool Constant>
class ArrayViewT
{
    public:

        using elementType=typename config_tree::ValueType<T>::type;
        using elementReturnType=typename config_tree::ValueType<T>::origin;

        using arrayT=typename config_tree::ValueType<T>::arrayType;
        constexpr static auto arrayTypeC=hana::if_(hana::bool_c<Constant>,hana::type_c<const arrayT>,hana::type_c<arrayT>);
        using arrayType=typename decltype(arrayTypeC)::type*;
        using arrayTypeRef=typename decltype(arrayTypeC)::type&;

        static arrayT* defaultArray()
        {
            static arrayT arr;
            return &arr;
        }

        ArrayViewT():m_arrayPtr(defaultArray())
        {}

        ArrayViewT(arrayTypeRef array):m_arrayPtr(&array)
        {}

        size_t size() const noexcept
        {
            return m_arrayPtr->size();
        }

        bool empty() const noexcept
        {
            return m_arrayPtr->empty();
        }

        size_t capacity() const noexcept
        {
            return m_arrayPtr->capacity();
        }

        void reserve(size_t count)
        {
            m_arrayPtr->reserve(count);
        }

        void resize(size_t count)
        {
            m_arrayPtr->resize(count);
        }

        template<typename T1>
        void resize(size_t count, const T1& value)
        {
            m_arrayPtr->resize(count, static_cast<elementType>(value));
        }

        void clear()
        {
            m_arrayPtr->clear();
        }

        void erase(size_t index)
        {
            auto it=m_arrayPtr->begin()+index;
            m_arrayPtr->erase(it);
        }

        template<typename T1>
        void set(size_t index, const T1& value)
        {
            (*m_arrayPtr)[index]=static_cast<elementType>(value);
        }

        template<typename T1>
        void set(size_t index, T1&& value)
        {
            (*m_arrayPtr)[index]=std::forward<T1>(value);
        }

        template<typename T1>
        void insert(size_t index, const T1& value)
        {
            m_arrayPtr->insert(m_arrayPtr->begin()+index,static_cast<elementType>(value));
        }

        template<typename T1>
        void insert(size_t index, T1&& value)
        {
            m_arrayPtr->insert(m_arrayPtr->begin()+index,std::forward<T1>(value));
        }

        template<typename T1>
        void append(const T1& value)
        {
            m_arrayPtr->push_back(static_cast<elementType>(value));
        }

        template<typename T1>
        void append(T1&& value)
        {
            m_arrayPtr->push_back(std::forward<T1>(value));
        }

        template <typename... Args>
        auto emplace(size_t index, Args&&... args) -> decltype(auto)
        {
            m_arrayPtr->emplace(m_arrayPtr->begin()+index,std::forward<Args>(args)...);
            return at(index);
        }

        template <typename... Args>
        auto emplaceBack(Args&&... args) -> decltype(auto)
        {
            m_arrayPtr->emplace_back(std::forward<Args>(args)...);
            return at(size()-1);
        }

        auto at(size_t index) const -> decltype(auto);
        auto at(size_t index) -> decltype(auto);

        auto operator[](size_t index) const -> decltype(auto)
        {
            return at(index);
        }

        auto operator[](size_t index) -> decltype(auto)
        {
            return at(index);
        }

        Error merge(ArrayViewT&& other, config_tree::ArrayMerge mode);

    private:

        arrayType m_arrayPtr;
};

template <typename T>
using ConstArrayView=ArrayViewT<T,true>;

template <typename T>
using ArrayView=ArrayViewT<T,false>;

//! Basic element of configuration tree.
class HATN_BASE_EXPORT ConfigTreeValue
{
    public:

        using Type=config_tree::Type;
        using NumericType=config_tree::NumericType;

        ConfigTreeValue()
            :m_type(Type::None),
             m_defaultType(Type::None)
        {}

        ConfigTreeValue(
                ConfigTreeValue&& other
            ) : m_type(other.m_type),
                m_defaultType(other.m_defaultType),
                m_value(std::move(other.m_value)),
                m_defaultValue(std::move(other.m_defaultValue))
        {
            other.reset();
            other.resetDefault();
        }

        ConfigTreeValue& operator =(ConfigTreeValue&& other)
        {
            if (&other!=this)
            {
                m_type=other.m_type;
                m_defaultType=other.m_defaultType;
                m_value=std::move(other.m_value);
                m_defaultValue=std::move(other.m_defaultValue);
                other.reset();
                other.resetDefault();
            }
            return *this;
        }

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

        NumericType numericType(bool orDefault=false) const noexcept
        {
            auto isSet=static_cast<bool>(m_value);
            if (!isSet && orDefault)
            {
                return m_defaultNumericType;
            }
            return m_numericType;
        }

        NumericType defaultNumericType() const noexcept
        {
            return m_defaultNumericType;
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
            m_numericType=NumericType::None;
            m_value.reset();
        }

        void resetDefault() noexcept
        {
            m_defaultType=Type::None;
            m_defaultNumericType=NumericType::None;
            m_defaultValue.reset();
        }

        template <typename T> void set(T value);
        template <typename T> void setDefault(T value);

        template <typename T> auto as() const noexcept -> decltype(auto);
        template <typename T> auto as(common::Error& ec) const noexcept -> decltype(auto);
        template <typename T> auto asEx() const -> decltype(auto);

        template <typename T> auto getDefault() const noexcept -> decltype(auto);
        template <typename T> auto getDefault(common::Error& ec) const noexcept -> decltype(auto);
        template <typename T> auto getDefaultEx() const -> decltype(auto);

        template <typename T>
        auto toArray() -> decltype(auto)
        {            
            using valueType=typename config_tree::ValueType<T>::arrayType;
            constexpr static auto typeId=config_tree::ValueType<T>::arrayId;

            if (m_type==typeId)
            {
                return asArray<T>();
            }

            m_type=typeId;
            m_numericType=config_tree::NumericType::Int64;
            m_value.reset();
            m_value.emplace(valueType{});
            return asArray<T>();
        }

        template <typename T> Result<ConstArrayView<T>> asArray() const noexcept;
        template <typename T> ConstArrayView<T> asArray(common::Error& ec) const noexcept
        {
            auto r=asArray<T>();
            HATN_RESULT_EC(r,ec)
            return r.takeValue();
        }
        template <typename T> ConstArrayView<T> asArrayEx() const
        {
            auto r=asArray<T>();
            HATN_RESULT_THROW(r)
            return r.takeValue();
        }

        template <typename T> Result<ArrayView<T>> asArray() noexcept;
        template <typename T> ArrayView<T> asArray(common::Error& ec) noexcept
        {
            auto r=asArray<T>();
            HATN_RESULT_EC(r,ec)
            return r.takeValue();
        }
        template <typename T> ArrayView<T> asArrayEx()
        {
            auto r=asArray<T>();
            HATN_RESULT_THROW(r)
            return r.takeValue();
        }

        Result<config_tree::MapT&> toMap()
        {
            if (m_type==Type::Map)
            {
                return asMap();
            }
            m_type=Type::Map;
            m_numericType=NumericType::None;
            m_value.reset();
            m_value.emplace(config_tree::MapT{});
            return asMap();
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
            HATN_RESULT_EC(r,ec)
            return r.takeValue();
        }

        config_tree::MapT& asMap(common::Error& ec) noexcept
        {
            auto&& r = asMap();
            HATN_RESULT_EC(r,ec)
            return r.takeValue();
        }

        const config_tree::MapT& asMapEx() const
        {
            auto&& r = asMap();
            HATN_RESULT_THROW(r)
            return r.takeValue();
        }

        config_tree::MapT& asMapEx()
        {
            auto&& r = asMap();
            HATN_RESULT_THROW(r)
            return r.takeValue();
        }

    protected:

        const config_tree::HolderT& value() const
        {
            return m_value;
        }

        config_tree::HolderT& value()
        {
            return m_value;
        }

        const config_tree::HolderT& defaultValue() const
        {
            return m_defaultValue;
        }

        config_tree::HolderT& defaultValue()
        {
            return m_defaultValue;
        }

        void setValue(config_tree::HolderT&& value)
        {
            m_value=std::move(value);
        }

        void setDefaultValue(config_tree::HolderT&& value)
        {
            m_defaultValue=std::move(value);
        }

    private:

        Type m_type;
        Type m_defaultType;
        config_tree::HolderT m_value;
        config_tree::HolderT m_defaultValue;

        NumericType m_numericType;
        NumericType m_defaultNumericType;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEVALUE_H
