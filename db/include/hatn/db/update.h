/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/update.h
  *
  * Contains types for db update operations.
  *
  */

/****************************************************************************/

#ifndef HATNDBUPDATE_H
#define HATNDBUPDATE_H

#include <hatn/validator/utils/reference_wrapper.hpp>

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>

HATN_DB_NAMESPACE_BEGIN

template <typename T>
using VectorT=query::VectorT<T>;

using VectorString=query::VectorString;

template <typename T>
using Vector=query::Vector<T>;

using Bytes=common::ConstDataBuf;

#define HATN_DB_UPDATE_VALUE_TYPES(DO) \
    DO(bool), \
    DO(int8_t), \
    DO(int16_t), \
    DO(int32_t), \
    DO(int64_t), \
    DO(uint8_t), \
    DO(uint16_t), \
    DO(uint32_t), \
    DO(uint64_t), \
    DO(float), \
    DO(double), \
    DO(String), \
    DO(common::DateTime), \
    DO(common::Date), \
    DO(common::Time), \
    DO(common::DateRange), \
    DO(ObjectId)

#define HATN_DB_UPDATE_VALUE_TYPE_IDS(DO) \
    DO(Bool), \
    DO(Int8_t), \
    DO(Int16_t), \
    DO(Int32_t), \
    DO(Int64_t), \
    DO(Uint8_t), \
    DO(Uint16_t), \
    DO(Uint32_t), \
    DO(Uint64_t), \
    DO(Float), \
    DO(Double), \
    DO(String), \
    DO(DateTime), \
    DO(Date), \
    DO(Time), \
    DO(DateRange), \
    DO(ObjectId)

#define HATN_DB_UPDATE_VALUE_TYPE(Type) \
    Type, \
    Vector<Type>

#define HATN_DB_UPDATE_VALUE_TYPE_ID(Type) \
    Type, \
    Vector##Type

namespace update
{

using String=query::String;

enum class Operator : uint8_t
{
    set,
    unset,
    inc,
    push,
    pop,
    push_unique,
    replace_element,
    erase_element,
    inc_element
};

constexpr const auto set=Operator::set;
constexpr const auto unset=Operator::unset;
constexpr const auto inc=Operator::inc;
constexpr const auto push=Operator::push;
constexpr const auto pop=Operator::pop;
constexpr const auto push_unique=Operator::push_unique;
constexpr const auto replace_element=Operator::replace_element;
constexpr const auto erase_element=Operator::erase_element;
constexpr const auto inc_element=Operator::inc_element;

using ValueVariant=lib::variant<
    HATN_DB_UPDATE_VALUE_TYPES(HATN_DB_UPDATE_VALUE_TYPE)
>;

enum class ValueType : uint8_t
{
    HATN_DB_UPDATE_VALUE_TYPE_IDS(HATN_DB_UPDATE_VALUE_TYPE_ID)
};

using Operand=query::ValueT<ValueVariant,ValueType>;

struct Field
{
    template <typename T>
    Field(
            FieldPath path,
            Operator op,
            T&& value
        ) : path(std::move(path)),
            op(op),
            value(std::forward<T>(value))
    {
        check();
    }

    Field(
            FieldPath path,
            Operator op
        ) : path(std::move(path)),
            op(op)
    {
        check();
    }

    FieldPath path;
    Operator op;
    Operand value;

    private:

        void check() const
        {
            Assert(path.size()>0,"FieldPath must be not null");
            auto firstFieldPathItem=path.at(0);
            Assert(firstFieldPathItem.fieldId!=ObjectIdFieldId,"Cannot update object::_id");
            Assert(firstFieldPathItem.fieldId!=CreatedAtFieldId,"Cannot update object::created_at");
            Assert(firstFieldPathItem.fieldId!=UpdatedAtFieldId,"Cannot update object::created_at");

            //! @todo Check combination of operator and operand
        }
};

constexpr makePathT path{};

struct fieldT
{
    template <typename T>
    auto wrapValue(T&& val) const -> decltype(auto)
    {
        using type=std::decay_t<T>;
        using isVecT=query::IsVector<T>;

        static const auto isLval=std::is_lvalue_reference<T>{};
        static const auto isStrView=
            hana::bool_c<
                std::is_same<lib::string_view,type>::value
            ||
            std::is_same<const char*,T>::value
                >;

        static_assert(
            decltype(isLval)::value || decltype(isStrView)::value || !isVecT::value,
            "Do not use temporary/rvalue strings or vectors as a update field value"
            );

        if constexpr (std::is_convertible<T,String>::value)
        {
            return String(val);
        }
        else
        {
            if constexpr (isVecT::value
                          &&
                          !std::is_same<type,std::vector<char>>::value
                          &&
                          !std::is_same<type,common::pmr::vector<char>>::value
                          &&
                          !std::is_same<type,common::ByteArray>::value
                          &&
                          !hana::is_a<common::VectorOnStackTag,type>
                          )
            {
                return std::cref(val);
            }
            else
            {
                return hana::id(std::forward<T>(val));
            }
        }
    }

    template <typename T1, typename T2>
    Field operator ()(T1&& path_,Operator op,T2&& value) const
    {
        if constexpr (hana::is_a<HATN_DATAUNIT_NAMESPACE::FieldTag,T1>)
        {
            return Field{path(std::forward<T1>(path_)),op,wrapValue(std::forward<T2>(value))};
        }
        else
        {
            if constexpr (hana::is_a<NestedFieldTag,T1>)
            {
                return Field{path_.fieldPath(),op,wrapValue(std::forward<T2>(value))};
            }
            else
            {
                static_assert(std::is_same<FieldPath,std::decay_t<T1>>::value,"Invalid path type");
                return Field{std::forward<T1>(path_),op,wrapValue(std::forward<T2>(value))};
            }
        }
    }

    template <typename T1>
    Field operator ()(T1&& path_,Operator op) const
    {
        if constexpr (hana::is_a<HATN_DATAUNIT_NAMESPACE::FieldTag,T1>)
        {
            return Field{path(std::forward<T1>(path_)),op};
        }
        else
        {
            if constexpr (hana::is_a<NestedFieldTag,T1>)
            {
                return Field{path_.fieldPath(),op};
            }
            else
            {
                static_assert(std::is_same<FieldPath,std::decay_t<T1>>::value,"Invalid path type");
                return Field{std::forward<T1>(path_),op};
            }
        }
    }
};
constexpr fieldT field{};

constexpr const size_t PreallocatedOpsCount=8;
using FieldsVector=common::VectorOnStack<Field,PreallocatedOpsCount>;

using Request=FieldsVector;

//! @todo Implement operations with repeated subunits
struct makeRequestT
{
    template <typename ...Fields>
    auto operator()(Fields&&... fields) const
    {
        Request r;
        r.reserve(sizeof...(fields));
        hana::for_each(
            hana::make_tuple(std::forward<Fields>(fields)...),
            [&r](auto&& field)
            {
                r.emplace_back(std::forward<decltype(field)>(field));
            }
        );
        return r;
    }
};
constexpr makeRequestT makeRequest{};
constexpr makeRequestT request{};

struct allocateRequestT
{
    template <typename ...Fields>
    auto operator()(common::pmr::AllocatorFactory* factory, Fields&&... fields) const
    {
        auto r=factory->createObject<Request>();
        r->reserve(sizeof...(fields));
        hana::for_each(
            hana::make_tuple(std::forward<Fields>(fields)...),
            [&r](auto&& field)
            {
                r->emplace_back(std::forward<decltype(field)>(field));
            }
            );
        return r;
    }
};
constexpr allocateRequestT allocateRequest{};

enum class ModifyReturn : int
{
    None=0,
    Before=1,
    After=2
};

constexpr auto ReturnNone=ModifyReturn::None;
constexpr auto ReturnAfter=ModifyReturn::After;
constexpr auto ReturnBefore=ModifyReturn::Before;

} // namespace update

HATN_DB_NAMESPACE_END

#endif // HATNDBUPDATE_H
