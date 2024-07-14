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
#include <hatn/common/pmr/string.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/namespace.h>

HATN_DB_NAMESPACE_BEGIN

namespace query
{

struct Null
{
    bool operator < (const Null&) const noexcept
    {
        return false;
    }
};

struct First
{
    bool operator < (const First&) const noexcept
    {
        return false;
    }
};

struct Last
{
    bool operator < (const Last&) const noexcept
    {
        return false;
    }
};

constexpr size_t StringPreallocatedSize=HATN_COMMON_NAMESPACE::pmr::StringPreallocatedSize;

using String=HATN_COMMON_NAMESPACE::pmr::StringT<StringPreallocatedSize>;

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
    Open,
    First,
    Last
};

inline IntervalType reverseIntervalType(IntervalType type)
{
    return (type==IntervalType::Closed) ? IntervalType::Open : IntervalType::Closed;
}

template <typename T>
struct Interval
{
    using hana_tag=IntervalTag;

    using Type=IntervalType;
    using ValueType=T;

    struct Endpoint
    {
        T value;
        Type type;

        Endpoint() : type(IntervalType::Open)
        {}

        template <typename T1>
        Endpoint(T1&& v, Type t)
            : value(std::forward<T1>(v)),
              type(t)
        {}

        bool less(const Endpoint& other, bool selfFrom, bool otherFrom) const noexcept
        {
            static std::less<T> lt{};

            if (type==IntervalType::First && other.type!=IntervalType::First)
            {
                return true;
            }

            if (type==IntervalType::Last && other.type!=IntervalType::Last)
            {
                return false;
            }

            if (other.type==IntervalType::First)
            {
                return false;
            }

            if (other.type==IntervalType::Last)
            {
                return true;
            }

            if (lt(value,other.value))
            {
                return true;
            }

            if (lt(other.value,value))
            {
                return false;
            }

            // values are equal

            if (type==other.type)
            {
                return false;
            }

            // types are not equal

            if (selfFrom)
            {
                if (otherFrom)
                {
                    if (type==IntervalType::Closed)
                    {
                        // other is open which means that other will be always greater than this
                        return true;
                    }
                }
                else
                {
                    if (other.type==IntervalType::Closed)
                    {
                        // other is closed which means that other will be always greater than this
                        return true;
                    }
                }
            }
            else
            {
                if (!otherFrom)
                {
                    if (other.type==IntervalType::Closed)
                    {
                        // other is closed which means that other will be always greater than this
                        return true;
                    }
                }
                else
                {
                    if (type==IntervalType::Closed)
                    {
                        // other is open which means that other will be always greater than this
                        return true;
                    }
                }
            }

            // done
            return false;
        }
    };

    template <typename T1, typename T2>
    Interval(
            T1&& from,
            IntervalType fromType,
            T2&& to,
            IntervalType toType
        ):from(std::forward<T1>(from),fromType),
          to(std::forward<T2>(to),toType)
    {}

    Endpoint from;
    Endpoint to;

    static void sortAndMerge(common::pmr::vector<Interval<T>>& vec, Order order);

    bool intersects(const Interval<T>& other) const noexcept;

    bool contains(const T& value) const noexcept
    {
        if (
            (from.type==IntervalType::First)
            ||
            ((from.type==IntervalType::Open) && (value > from.value))
             ||
            ((from.type==IntervalType::Closed) && (value >= from.value))
            )
        {
            if (
                (to.type==IntervalType::Last)
                ||
                ((to.type==IntervalType::Open) && (value < to.value))
                ||
                ((to.type==IntervalType::Closed) && (value <= from.value))
                )
            {
                return true;
            }
        }
        return false;
    }
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
        DO(String), \
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

template <typename HandlerT>
struct IntervalVisitor
{
    HandlerT handler;

    template <typename T>
    Error operator()(const query::Interval<T>& interval) const
    {
        return handler(interval);
    }

    template <typename T>
    Error operator()(const T&) const
    {
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    IntervalVisitor(T&& fn):handler(std::forward<T>(fn))
    {}
};

template <typename HandlerT>
struct VectorVisitor
{
    template <typename T>
    Error operator()(const common::pmr::vector<T>& vec) const
    {
        return handler(vec);
    }

    template <typename T>
    Error operator()(const T&) const
    {
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
};

template <typename HandlerT>
struct VectorItemVisitor
{
    template <typename T>
    Error operator()(const common::pmr::vector<T>& vec) const
    {
        for (auto&& it: vec)
        {
            auto ec=handler(it);
            HATN_CHECK_EC(ec)
        }
        return OK;
    }

    template <typename T>
    Error operator()(const T&) const
    {
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorItemVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
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
            // scalar value type is each 1st in the type enum
            return (static_cast<int>(typeId())-1)%4==0;
        }

        bool isVectorType() const noexcept
        {
            // scalar value type is each 2nd in the type enum
            return (static_cast<int>(typeId())-2)%4==0;
        }

        bool isVectorIntervalType() const noexcept
        {
            // scalar value type is each 3th in the type enum
            return (static_cast<int>(typeId())-3)%4==0;
        }

        IntervalType fromIntervalType() const noexcept
        {
            constexpr static const detail::IntervalTypeVisitor<hana::true_> v{};
            return common::lib::variantVisit(v,m_value);
        }

        IntervalType toIntervalType() const noexcept
        {
            constexpr static const detail::IntervalTypeVisitor<hana::false_> v{};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error eachVectorItem(const HandlerT& handler) const
        {
            detail::VectorItemVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error handleInterval(const HandlerT& handler) const
        {
            detail::IntervalVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error handleVector(const HandlerT& handler) const
        {
            detail::VectorVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error eachVectorItem(const HandlerT& handler)
        {
            detail::VectorItemVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error handleInterval(const HandlerT& handler)
        {
            detail::IntervalVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error handleVector(const HandlerT& handler)
        {
            detail::VectorVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename T>
        const T& as() const
        {
            return common::lib::variantGet<T>(m_value);
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
        if (value.isVectorType())
        {
            ok=!isScalarOp() || op==Operator::neq;
        }
        else if (value.isScalarType())
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

} // namespace query

HATN_DB_NAMESPACE_END

namespace std {

template <typename T>
struct less<HATN_DB_NAMESPACE::query::Interval<T>>
{
    bool operator() (const HATN_DB_NAMESPACE::query::Interval<T>& l, const HATN_DB_NAMESPACE::query::Interval<T>& r) const noexcept
    {
        // if left from less than right from then result is less
        if (l.from.less(r.from,true,true))
        {
            return true;
        }

        // if right from less than left from then result is not less
        if (r.from.less(l.from,true,true))
        {
            return false;
        }

        // left and right from are equal

        // if left to less than right to then result is less
        if (l.to.less(r.to,false,false))
        {
            return true;
        }

        // if right to less than left to then result is not less
        // not needed
        // if (r.to.less(l.to,false,false))
        // {
        //     return false;
        // }

        // result is equal or not less
        return false;
    }
};

} // namespace std

HATN_DB_NAMESPACE_BEGIN

namespace query
{

template <typename T>
bool Interval<T>::intersects(const Interval<T>& other) const noexcept
{
    if (to.less(other.from,false,true) || other.to.less(from,false,true))
    {
        return false;
    }

    return true;
}

template <typename T>
void Interval<T>::sortAndMerge(common::pmr::vector<Interval<T>>& vec, Order order)
{
    // sort vector
    std::sort(
        vec.begin(),
        vec.end(),
        std::less<Interval<T>>{}
    );

    // merge intervals
    auto it=vec.begin();
    for (;;)
    {
        auto it1=it;
        ++it1;
        if (it1==vec.end())
        {
            break;
        }

        if (it->intersects(*it1))
        {
            if (it1->from.less(it->from,true,true))
            {
                it->from=std::move(it1->from);
            }
            if (it->to.less(it1->to,false,false))
            {
                it->to=std::move(it1->to);
            }
            vec.erase(it1);
        }
        else
        {
            ++it;
        }

        if (it==vec.end())
        {
            break;
        }
    }

    // reverse vector if order is descending
    if (order==query::Order::Desc)
    {
        std::reverse(vec.begin(),vec.end());
    }
}

} // namespace query

HATN_DB_NAMESPACE_END

#endif // HATNDBQUERY_H
