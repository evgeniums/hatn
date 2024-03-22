/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/detail/configtreevalue.ipp
  *
  * Contains definitions of configuration tree value types and functions.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREEVALUE_IPP
#define HATNCONFIGTREEVALUE_IPP

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
        return baseErrorResult(BaseError::INVALID_TYPE);
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
    using storageType=typename config_tree::Storage<id>::type;

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
    using storageType=typename config_tree::Storage<id>::type;

    template <typename ValueT>
    static void set(HolderT& holder, Type& typeId, ValueT value) noexcept
    {
        auto val=static_cast<storageType>(value);
        ValueSetterMove<id>::set(holder,typeId,std::move(val));
    }
};

template <typename T, typename T1=void> struct ValueSetter
{
    constexpr static auto id=Type::None;
    using storageType=typename config_tree::Storage<id>::type;

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
template <typename T, typename StorageT, typename =void> struct ArrayAt
{
    template <typename ArrayT>
    constexpr static auto f(ArrayT&& array, size_t index) -> decltype(auto)
    {
        return array->at(index);
    }
};

template <typename T, typename StorageT> struct ArrayAt<T,StorageT, std::enable_if_t<!std::is_same<T,StorageT>::value>>
{
    template <typename ArrayT>
    constexpr static auto f(ArrayT&& array, size_t index) -> decltype(auto)
    {
        auto val=static_cast<T>(array->at(index));
        return val;
    }
};

//---------------------------------------------------------------

} // namespace config_tree_detail

//---------------------------------------------------------------
template <typename T, bool Constant>
auto ArrayViewT<T,Constant>::at(size_t index) const -> decltype(auto)
{
    return config_tree_detail::ArrayAt<elementReturnType,elementType>::f(m_arrayPtr,index);
}

template <typename T, bool Constant>
auto ArrayViewT<T,Constant>::at(size_t index) -> decltype(auto)
{
    return config_tree_detail::ArrayAt<elementReturnType,elementType>::f(m_arrayPtr,index);
}

template <typename T, bool Constant>
Error ArrayViewT<T,Constant>::merge(ArrayViewT<T,Constant>&& other, config_tree::ArrayMerge mode)
{
    switch (mode)
    {
        case(config_tree::ArrayMerge::Merge):
        {
            for (size_t i=0;i<size();i++)
            {
                if (i==other.size())
                {
                    break;
                }

                auto self=this;
                return hana::eval_if(
                    hana::equal(hana::type_c<T>,hana::type_c<ConfigTree>),
                    [&](auto _)
                    {
                        return _(self)->m_arrayPtr->at(_(i))->merge(std::move(*(_(other).at(_(i)))),ConfigTreePath(),_(mode));
                    },
                    [&](auto _)
                    {
                        (*(_(self)->m_arrayPtr))[_(i)]=std::move(_(other).at(_(i)));
                        return Error();
                    }
                );
            }

            if (other.size()>size())
            {
                size_t offset=other.size()-size();
                m_arrayPtr->insert(m_arrayPtr->end(), std::make_move_iterator(other.m_arrayPtr->begin()+offset),
                                   std::make_move_iterator(other.m_arrayPtr->end()));
            }
            break;
        }
        case(config_tree::ArrayMerge::Append):
        {
            m_arrayPtr->insert(m_arrayPtr->end(), std::make_move_iterator(other.m_arrayPtr->begin()),
                               std::make_move_iterator(other.m_arrayPtr->end()));

            break;
        }
        case(config_tree::ArrayMerge::Prepend):
        {
            m_arrayPtr->insert(m_arrayPtr->begin(), std::make_move_iterator(other.m_arrayPtr->begin()),
                               std::make_move_iterator(other.m_arrayPtr->end()));

            break;
        }

        default:break;
    }

    return OK;
}

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
            return Result<valueT>{baseErrorResult(BaseError::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_value));
    }

    if (static_cast<bool>(m_defaultValue))
    {
        if (expectedTypeId!=m_defaultType)
        {
            return Result<valueT>{baseErrorResult(BaseError::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    }

    return Result<valueT>{baseErrorResult(BaseError::VALUE_NOT_SET)};
}

template <typename T>
auto ConfigTreeValue::as(common::Error& ec) const noexcept -> decltype(auto)
{
    auto&& r = as<T>();
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

template <typename T>
auto ConfigTreeValue::asEx() const  -> decltype(auto)
{
    auto&& r = as<T>();
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

template <typename T>
auto ConfigTreeValue::getDefault() const noexcept -> decltype(auto)
{
    using valueT=decltype(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    auto expectedTypeId=config_tree_detail::ValueSetter<std::decay_t<T>>::id;
    if (static_cast<bool>(m_defaultValue))
    {
        if (expectedTypeId!=m_defaultType)
        {
            return Result<valueT>{baseErrorResult(BaseError::INVALID_TYPE)};
        }
        return makeResult(config_tree_detail::valuesAs<T>::f(m_defaultValue));
    }
    return Result<valueT>{baseErrorResult(BaseError::VALUE_NOT_SET)};
}

template <typename T>
auto ConfigTreeValue::getDefault(common::Error& ec) const noexcept -> decltype(auto)
{
    auto&& r = getDefault<T>();
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

template <typename T>
auto ConfigTreeValue::getDefaultEx() const -> decltype(auto)
{
    auto&& r = getDefault<T>();
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

//---------------------------------------------------------------

template <typename T>
void ConfigTreeValue::set(T value)
{
    using type=std::decay_t<T>;
    config_tree_detail::ValueSetter<type>::set(m_value,m_type,std::move(value));
    m_numericType=config_tree::ValueType<type>::numericId();
}

template <typename T>
void ConfigTreeValue::setDefault(T value)
{
    using type=std::decay_t<T>;
    config_tree_detail::ValueSetter<type>::set(m_defaultValue,m_defaultType,std::move(value));
    m_defaultNumericType=config_tree::ValueType<type>::numericId();
}

//---------------------------------------------------------------

template <typename T>
Result<ConstArrayView<T>> ConfigTreeValue::asArray() const noexcept
{
    using valueType=typename config_tree::ValueType<T>::arrayType;
    auto expectedTypeId=config_tree::ValueType<T>::arrayId;

    if (static_cast<bool>(m_value))
    {
        if (expectedTypeId!=m_type)
        {
            return baseErrorResult(BaseError::INVALID_TYPE);
        }

        const auto& arrayRef=common::lib::variantGet<valueType>(m_value.value());
        return emplaceResult<ConstArrayView<T>>(arrayRef);
    }

    return baseErrorResult(BaseError::VALUE_NOT_SET);
}

template <typename T>
Result<ArrayView<T>> ConfigTreeValue::asArray() noexcept
{
    using valueType=typename config_tree::ValueType<T>::arrayType;
    auto expectedTypeId=config_tree::ValueType<T>::arrayId;

    if (static_cast<bool>(m_value))
    {
        if (expectedTypeId!=m_type)
        {
            return baseErrorResult(BaseError::INVALID_TYPE);
        }

        auto& arrayRef=common::lib::variantGet<valueType>(m_value.value());
        return emplaceResult<ArrayView<T>>(arrayRef);
    }

    return baseErrorResult(BaseError::VALUE_NOT_SET);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEVALUE_IPP
