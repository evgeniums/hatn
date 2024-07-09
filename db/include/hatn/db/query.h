/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/query.h
  *
  * Contains declarations of db queries.
  *
  */

/****************************************************************************/

#ifndef HATNDBQUERY_H
#define HATNDBQUERY_H

#include <boost/hana.hpp>

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

namespace query
{

struct Null{};
struct First{};
struct Last{};

enum class Operator : uint8_t
{
    eq,
    gt,
    gte,
    lt,
    lte,
    in,
    neq,
    nin
};

enum class Order : uint8_t
{
    Asc,
    Desc
};

struct IntervalTag{};

enum class IntervalType : uint8_t
{
    Closed,
    Open
};

template <typename T>
struct Interval
{
    using hana_tag=IntervalTag;

    using Type=IntervalType;

    struct Endpoint
    {
        T value;
        Type type;
    };

    Endpoint from;
    Endpoint to;
};

#define HATN_DB_QUERY_VALUE_TYPES(DO) \
        DO(Null), \
        DO(First), \
        DO(Last), \
        DO(bool), \
        DO(int8_t), \
        DO(int16_t), \
        DO(int32_t), \
        DO(int64_t), \
        DO(uint8_t), \
        DO(uint16_t), \
        DO(uint32_t), \
        DO(uint64_t), \
        DO(common::pmr::string), \
        DO(common::DateTime), \
        DO(common::Date), \
        DO(common::Time), \
        DO(common::DateRange), \
        DO(ObjectId)

#define HATN_DB_QUERY_VALUE_TYPE_IDS(DO) \
        DO(Null), \
        DO(First), \
        DO(Last), \
        DO(Bool), \
        DO(Int8_t), \
        DO(Int16_t), \
        DO(Int32_t), \
        DO(Int64_t), \
        DO(Uint8_t), \
        DO(Uint16_t), \
        DO(Uint32_t), \
        DO(Uint64_t), \
        DO(String), \
        DO(DateTime), \
        DO(Date), \
        DO(Time), \
        DO(DateRange), \
        DO(ObjectId)

#define HATN_DB_QUERY_VALUE_TYPE(Type) \
        Type, \
        Interval<Type>, \
        common::pmr::vector<Type>, \
        common::pmr::vector<Interval<Type>>

#define HATN_DB_QUERY_VALUE_TYPE_ID(Type) \
        Type, \
        Interval##Type, \
        Vector##Type, \
        VectorInterval##Type

namespace detail
{

template <typename EndpointT>
struct IntervalTypeVisitor
{
    constexpr static const EndpointT endpoint{};

    template <typename T>
    IntervalType operator()(const query::Interval<T>& value) const
    {
        return hana::if_(
            endpoint,
            value.from.type,
            value.to.type
        );
    }

    template <typename T>
    IntervalType operator()(const T&) const
    {
        return IntervalType::Open;
    }
};

}

class Value
{
    public:

        using type=lib::variant<
            HATN_DB_QUERY_VALUE_TYPES(HATN_DB_QUERY_VALUE_TYPE)
        >;

        enum class Type : uint8_t
        {
            HATN_DB_QUERY_VALUE_TYPE_IDS(HATN_DB_QUERY_VALUE_TYPE_ID)
        };

        template <typename T>
        Value(T&& val) : m_value(std::forward<T>(val))
        {}

        Type typeId() const noexcept
        {
            return static_cast<Type>(lib::variantIndex(m_value));
        }

        bool isType(Type t) const noexcept
        {
            return typeId()==t;
        }

        type& operator()() noexcept
        {
            return m_value;
        }

        const type& operator()() const noexcept
        {
            return m_value;
        }

        bool isScalarType() const noexcept
        {
            // scalar value type is each 4th in the type enum
            return static_cast<int>(typeId())%4==0;
        }

        bool isIntervalType() const noexcept
        {
            // scalar value type is each 2th in the type enum
            return static_cast<int>(typeId()-1)%4==0;
        }

        IntervalType fromIntervalType() const noexcept
        {
            constexpr static const detail::IntervalTypeVisitor<hana::true_> v{};
            return v(m_value);
        }

        IntervalType toIntervalType() const noexcept
        {
            constexpr static const detail::IntervalTypeVisitor<hana::false_> v{};
            return v(m_value);
        }

    private:

        type m_value;
};

using Operand=Value;

struct Field
{
    template <typename T>
    Field(
        const IndexFieldInfo& fieldInfo,
        Operator op,
        T&& value,
        Order order=Order::Asc
      ) : fieldInfo(&fieldInfo),
          op(op),
          value(std::forward<T>(value)),
          order(order)
    {
        checkOperator();
    }

    bool isScalarOp() const noexcept
    {
        return op!=Operator::in && op!=Operator::nin;
    }

    void checkOperator() const
    {
        bool ok=true;
        if (value.isScalarType())
        {
            ok=isScalarOp();
        }
        else
        {
            ok=!isScalarOp();
        }

        Assert(ok,"Invalid combination of operator and operand");
    }

    bool matchScalarOp(const Field& other) const noexcept
    {
        auto ok=op==other.op && order==other.order && isScalarOp();
        return ok;
    }

    const IndexFieldInfo* fieldInfo;
    Operator op;
    Operand value;
    Order order;
};

using TimePointInterval=Interval<uint32_t>;

} // namespace query

class HATN_DB_EXPORT IndexQuery
{
    public:

        static common::pmr::AllocatorFactory* defaultAllocatorFactory() noexcept
        {
            if (DefaultAllocatorFactory!=nullptr)
            {
                return DefaultAllocatorFactory;
            }
            return common::pmr::AllocatorFactory::getDefault();
        }

        IndexQuery(
                const IndexInfo& index,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
           )  : m_index(&index),
                m_fields(factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {}

        IndexQuery(
                const IndexInfo& index,
                std::initializer_list<query::Field> list,
                common::pmr::AllocatorFactory* factory=defaultAllocatorFactory()
            ) : m_index(&index),
                m_fields(std::move(list),factory->dataAllocator<query::Field>()),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        IndexQuery(
                const IndexInfo& index,
                common::pmr::vector<query::Field> fields
            ) : m_index(&index),
                m_fields(std::move(fields)),
                m_limit(DefaultLimit)
        {
            checkFields();
        }

        void setFields(common::pmr::vector<query::Field> fields)
        {
            m_fields=(std::move(fields));
            checkFields();
        }

        void setFields(std::initializer_list<query::Field> fields)
        {
            m_fields=(std::move(fields));
            checkFields();
        }

        const IndexInfo& index() const noexcept
        {
            return *m_index;
        }

        const auto& fields() const noexcept
        {
            return m_fields;
        }

        const auto& field(size_t pos) const
        {
            return m_fields[pos];
        }

        void setLimit(size_t limit) noexcept
        {
            m_limit=limit;
        }

        size_t limit() const noexcept
        {
            return m_limit;
        }

        void setFilterTimePoints(common::SharedPtr<common::pmr::vector<query::TimePointInterval>> filterTimepoints)
        {
            m_filterTimepoints=std::move(filterTimepoints);
        }

        const auto& filterTimepoints() const noexcept
        {
            return m_filterTimepoints;
        }

        static size_t defaultLimit() noexcept
        {
            return DefaultLimit;
        }

        static void setDefaultLimit(size_t limit) noexcept
        {
            DefaultLimit=limit;
        }

        static void setDefaultAllocatorFactory(common::pmr::AllocatorFactory* factory) noexcept
        {
            DefaultAllocatorFactory=factory;
        }

    private:

        static size_t DefaultLimit;
        static common::pmr::AllocatorFactory* DefaultAllocatorFactory;

        void checkFields() const
        {
            Assert(m_fields.size()<=m_index->fields().size(),"Number of fields must be less or equal to number of fields in the index");

            for (size_t i=0;i<m_fields.size();i++)
            {
                const auto& qF=m_fields[i];
                const auto& iF=m_index->fields()[i];
                if (iF.nested())
                {
                    Assert(iF.name()==qF.fieldInfo->name(),"Field name mismatches name of corresponding index field");
                }
                else
                {
                    Assert(iF.id()==qF.fieldInfo->id(),"Field ID mismatches ID of corresponding index field");
                }
            }
        }

        const IndexInfo* m_index;
        common::pmr::vector<query::Field> m_fields;
        size_t m_limit;
        common::SharedPtr<common::pmr::vector<query::TimePointInterval>> m_filterTimepoints;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBQUERY_H
