/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/query.h
  *
  * Contains declarations of db queries.
  *
  */

/****************************************************************************/

#ifndef HATNDBQUERY_H
#define HATNDBQUERY_H

#include <boost/hana.hpp>

#include <hatn/validator/utils/reference_wrapper.hpp>

#include <hatn/common/objectid.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

namespace query
{

struct NullT
{
    bool operator < (const NullT&) const noexcept
    {
        return false;
    }
};
constexpr NullT Null{};

struct FirstT
{
    bool operator < (const FirstT&) const noexcept
    {
        return false;
    }
};
constexpr FirstT First{};

struct LastT
{
    bool operator < (const LastT&) const noexcept
    {
        return false;
    }
};
constexpr LastT Last{};

using String=lib::string_view;

using Enum=int32_t;

template <typename T, typename = hana::when<true>>
struct ValueTypeTraits
{
    using type=T;
    constexpr static const auto is_string=hana::false_c;
};

template <typename T>
struct ValueTypeTraits<T, hana::when<std::is_enum<T>::value>>
{
    using type=Enum;
    constexpr static const auto is_string=hana::false_c;
};

template <typename T>
struct ValueTypeTraits<T, hana::when<hana::is_a<common::FixedByteArrayTag,T>>>
{
    using type=String;
    constexpr static const auto is_string=hana::true_c;
};

template <>
struct ValueTypeTraits<std::string>
{
    using type=String;
    constexpr static const auto is_string=hana::true_c;
};

template <>
struct ValueTypeTraits<common::pmr::string>
{
    using type=String;
    constexpr static const auto is_string=hana::true_c;
};

template <typename T>
struct ValueTypeTraits<T, hana::when<hana::is_a<common::StringOnStackTag,T>>>
{
    using type=String;
    constexpr static const auto is_string=hana::true_c;
};

struct BoolValue
{
    BoolValue(bool val) noexcept :m_val(val)
    {}

    operator bool() const noexcept
    {
        return m_val;
    }

    bool m_val;
};

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

constexpr auto eq=Operator::eq;
constexpr auto gt=Operator::gt;
constexpr auto gte=Operator::gte;
constexpr auto lt=Operator::lt;
constexpr auto lte=Operator::lte;
constexpr auto in=Operator::in;
constexpr auto neq=Operator::neq;
constexpr auto nin=Operator::nin;

enum class Order : uint8_t
{
    Asc,
    Desc
};

constexpr auto Asc=Order::Asc;
constexpr auto Desc=Order::Desc;

struct IntervalTag{};

enum class IntervalType : uint8_t
{
    Closed,
    Open,
    First,
    Last
};

inline IntervalType reverseIntervalType(IntervalType type) noexcept
{
    return (type==IntervalType::Closed) ? IntervalType::Open : IntervalType::Closed;
}

inline const char* intervalTypeToString(IntervalType type) noexcept
{
    switch(type)
    {
        case (IntervalType::Closed): return "closed"; break;
        case (IntervalType::Open): return "open"; break;
        case (IntervalType::First): return "first"; break;
        case (IntervalType::Last): return "last"; break;
    }
    return "";
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

        Endpoint(bool v, Type t)
            : value(BoolValue(v)),
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
            IntervalType toType,
            std::enable_if_t<!std::is_enum<std::decay_t<T1>>::value>* =nullptr
        ):from(std::forward<T1>(from),fromType),
          to(std::forward<T2>(to),toType)
    {
        static const auto isLval1=std::is_lvalue_reference<T1>{};
        static const auto isStrView1=
            hana::bool_c<
                std::is_same<lib::string_view,std::decay_t<T1>>::value
            ||
            std::is_same<const char*,T1>::value
                >;
        static const auto isStr1=ValueTypeTraits<T1>::is_string;
        static_assert(
            decltype(isLval1)::value || !decltype(isStr1)::value || decltype(isStrView1)::value,
            "Temporary/rvalue string must not be used as From"
            );

        static const auto isLval2=std::is_lvalue_reference<T2>{};
        static const auto isStrView2=
            hana::bool_c<
                std::is_same<lib::string_view,std::decay_t<T2>>::value
            ||
            std::is_same<const char*,T2>::value
                >;
        static const auto isStr2=ValueTypeTraits<T2>::is_string;
        static_assert(
            decltype(isLval2)::value || !decltype(isStr2)::value || decltype(isStrView2)::value,
            "Temporary/rvalue string must not be used as To"
            );
    }

    template <typename T1>
    Interval(
            T1&& from,
            IntervalType fromType,
            T1&& to,
            IntervalType toType,
            std::enable_if_t<std::is_enum<std::decay_t<T1>>::value>* =nullptr
        ): from(static_cast<Enum>(from),fromType),
           to(static_cast<Enum>(to),toType)
    {}

    Interval(
        bool from,
        IntervalType fromType,
        bool to,
        IntervalType toType
        ): from(BoolValue(from),fromType),
           to(BoolValue(to),toType)
    {}

    /**
     * @brief Interval constructor with From First
     * @param fromType
     * @param to
     * @param toType
     */
    template <typename T2>
    Interval(
        IntervalType fromType,
        T2&& to,
        IntervalType toType
        ): Interval(uint32_t(0),fromType,std::forward<T2>(to),toType)
    {
        Assert(fromType==IntervalType::First,"This constructor can be used only for interval from First");
    }

    /**
     * @brief Interval constructor with To Last
     * @param fromType
     * @param to
     * @param toType
     */
    template <typename T1>
    Interval(
        T1&& from,
        IntervalType fromType,
        IntervalType toType
        ) : Interval(std::forward<T1>(from),fromType,uint32_t(0),toType)
    {
        Assert(toType==IntervalType::Last,"This constructor can be used only for interval to Last");
    }

    /**
     * @brief Interval constructor with From First To Last
     * @param fromType
     * @param to
     * @param toType
     */
    Interval(
        IntervalType fromType,
        IntervalType toType
        ) : Interval(uint32_t(0),fromType,uint32_t(0),toType)
    {
        Assert(fromType==IntervalType::First && toType==IntervalType::Last,"This constructor can be used only for interval from First to Last");
    }

    Interval(
            T from,
            T to
        ) : Interval(std::move(from),IntervalType::Closed,std::move(to),IntervalType::Open)
    {}

    Endpoint from;
    Endpoint to;

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

//! Make default interval [from,to)
template <typename T>
auto makeInterval(T from, T to)
{
    return Interval<std::decay_t<T>>(std::move(from),std::move(to));
}

constexpr const size_t PreallocatedVectorSize=8;

template <typename T>
using VectorT=lib::variant<
    std::reference_wrapper<const std::vector<T>>,
    std::reference_wrapper<const common::pmr::vector<T>>,
    std::reference_wrapper<const common::VectorOnStack<T>>
    >;

using VectorString=lib::variant<
    std::reference_wrapper<const std::vector<std::string>>,
    std::reference_wrapper<const common::pmr::vector<std::string>>,
    std::reference_wrapper<const common::VectorOnStack<std::string>>,

    std::reference_wrapper<const std::vector<common::pmr::string>>,
    std::reference_wrapper<const common::pmr::vector<common::pmr::string>>,
    std::reference_wrapper<const common::VectorOnStack<common::pmr::string>>,

    std::reference_wrapper<const std::vector<lib::string_view>>,
    std::reference_wrapper<const common::pmr::vector<lib::string_view>>,
    std::reference_wrapper<const common::VectorOnStack<lib::string_view>>,

    std::reference_wrapper<const std::vector<common::StringOnStack>>,
    std::reference_wrapper<const common::pmr::vector<common::StringOnStack>>,
    std::reference_wrapper<const common::VectorOnStack<common::StringOnStack>>
    >;

struct VectorTag
{};

template <typename T, typename = hana::when<true>>
struct VectorTraits
{
    using hana_tag=VectorTag;
    using type=VectorT<T>;
};

template <typename T>
struct VectorTraits<T,hana::when<
                           std::is_same<T,std::string>::value
                           ||
                           std::is_same<T,lib::string_view>::value
                           ||
                           std::is_same<T,common::pmr::string>::value
                           ||
                           hana::is_a<common::StringOnStackTag,T>
                           >>
{
    using hana_tag=VectorTag;
    using type=VectorString;
};

template <typename T>
using Vector=typename VectorTraits<T>::type;

template <typename T>
auto makeEnumVector(const T& vec)
{
    common::VectorOnStack<int32_t,PreallocatedVectorSize> v;
    v.resize(vec.size());
    for (size_t i=0;i<vec.size();i++)
    {
        v[i]=static_cast<Enum>(vec[i]);
    }
    return v;
}

template <typename T, typename T1>
void fromEnumVector(const T& from, T1& to)
{
    to.resize(from.size());
    for (size_t i=0;i<from.size();i++)
    {
        to[i]=static_cast<Enum>(from[i]);
    }
}

#define HATN_DB_QUERY_VALUE_TYPES(DO) \
        DO(NullT), \
        DO(FirstT), \
        DO(LastT), \
        DO(BoolValue), \
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
        DO(NullT), \
        DO(FirstT), \
        DO(LastT), \
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


struct VectorIntervalTag{};

template <typename T>
struct VectorInterval
{
    using hana_tag=VectorIntervalTag;

    using type=T;
    using interval_type=Interval<type>;
    using vector_type=std::vector<interval_type>;

    vector_type value;
};

#define HATN_DB_QUERY_VALUE_TYPE(Type) \
        Type, \
        Interval<Type>, \
        Vector<Type>, \
        VectorInterval<Type>

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
    Error operator()(const VectorT<T>& vec) const
    {
        auto vis=[this](const auto& vCref)
        {
            return handler(vCref.get());
        };
        return common::lib::variantVisit(vis,vec);
    }

    Error operator()(const VectorString& vec) const
    {
        auto vis=[this](const auto& vCref)
        {
            return handler(vCref.get());
        };
        return common::lib::variantVisit(vis,vec);
    }

    //! @todo Fix it
    // template <typename T>
    // Error operator()(const VectorInterval<T>& vec) const
    // {
    //     auto vis=[this](const auto& v)
    //     {
    //         return handler(v);
    //     };
    //     return common::lib::variantVisit(vis,vec);
    // }

    template <typename T>
    Error operator()(const T&) const
    {
        Assert(false,"Invalid vector type in VectorVisitor");
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
};

template <typename HandlerT>
struct VectorIntervalVisitor
{
    template <typename T>
    Error operator()(const VectorInterval<T>& vec) const
    {
        return handler(vec);
    }

    template <typename T>
    Error operator()(const T&) const
    {
        Assert(false,"Invalid vector type in VectorIntervalVisitor");
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorIntervalVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
};

template <typename HandlerT>
struct VectorItemVisitor
{
    template <typename T>
    Error operator()(const VectorT<T>& vec) const
    {
        auto vis=[this](const auto& vCref)
        {
            for (auto&& it: vCref.get())
            {
                auto ec=handler(it);
                HATN_CHECK_EC(ec)
            }
            return Error{OK};
        };
        return common::lib::variantVisit(vis,vec);
    }

    Error operator()(const VectorString& vec) const
    {
        auto vis=[this](const auto& vCref)
        {
            for (auto&& it: vCref.get())
            {
                auto ec=handler(it);
                HATN_CHECK_EC(ec)
            }
            return Error{OK};
        };
        return common::lib::variantVisit(vis,vec);
    }

    template <typename T>
    Error operator()(const T&) const
    {
        Assert(false,"Invalid vector type in VectorItemVisitor");
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorItemVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
};

template <typename HandlerT>
struct VectorIntervalItemVisitor
{
    template <typename T>
    Error operator()(const VectorInterval<T>& vec) const
    {
        for (auto&& it: vec.value)
        {
            auto ec=handler(it);
            HATN_CHECK_EC(ec)
        }
        return Error{OK};
    }

    template <typename T>
    Error operator()(const T&) const
    {
        Assert(false,"Invalid vector type in VectorIntervalItemVisitor");
        return CommonError::INVALID_ARGUMENT;
    }

    template <typename T>
    VectorIntervalItemVisitor(T&& fn):handler(std::forward<T>(fn))
    {}

    HandlerT handler;
};

}

enum class ValueType : uint8_t
{
    HATN_DB_QUERY_VALUE_TYPE_IDS(HATN_DB_QUERY_VALUE_TYPE_ID)
};

template <typename VariantT, typename EnumT>
class ValueT
{
    public:

        using type=VariantT;
        using Type=EnumT;

        ValueT()=default;

        ValueT(bool val) : m_value(BoolValue(val))
        {}

        template <typename T>
        ValueT(const std::reference_wrapper<const std::vector<T>>& v) : m_value(Vector<T>{v})
        {}

        template <typename T>
        ValueT(const std::reference_wrapper<const common::pmr::vector<T>>& v) : m_value(Vector<T>{v})
        {}

        template <typename T>
        ValueT(const std::reference_wrapper<const common::VectorOnStack<T>>& v) : m_value(Vector<T>{v})
        {}

        ValueT(const char* value) : m_value(String(value))
        {}

        ValueT(const std::string& value) : m_value(String(value))
        {}

        ValueT(std::string&& value)= delete;

        ValueT(const common::pmr::string& value) : m_value(String(value.data(),value.size()))
        {}

        ValueT(common::pmr::string&& value)= delete;

        template <size_t CapacityS, bool ThrowOnOverflow>
        ValueT(const common::FixedByteArray<CapacityS,ThrowOnOverflow>& value) : m_value(String(value.data(),value.size()))
        {}

        template <size_t CapacityS, bool ThrowOnOverflow>
        ValueT(common::FixedByteArray<CapacityS,ThrowOnOverflow>&& value) = delete;

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        ValueT(const common::StringOnStackT<PreallocatedSize,FallbackAllocatorT>& value) : m_value(String(value.data(),value.size()))
        {}

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        ValueT(common::StringOnStackT<PreallocatedSize,FallbackAllocatorT>&& value) =delete;

        ValueT(const std::vector<char>& value) : m_value(String(value.data(),value.size()))
        {}

        ValueT(std::vector<char>&& value)= delete;

        ValueT(const common::ByteArray& value,
               void* =nullptr
               ) : m_value(String(value.data(),value.size()))
        {}

        // ValueT(common::ByteArray&& value)= delete;

        ValueT(const common::pmr::vector<char>& value) : m_value(String(value.data(),value.size()))
        {}

        ValueT(common::pmr::vector<char>&& value)= delete;

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        ValueT(const common::VectorOnStackT<char,PreallocatedSize,FallbackAllocatorT>& value) : m_value(String(value.data(),value.size()))
        {}

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        ValueT(common::VectorOnStackT<char,PreallocatedSize,FallbackAllocatorT>&& value) =delete;

        template <typename T>
        ValueT(const T& val,
               std::enable_if_t<!std::is_enum<std::decay_t<T>>::value>* =nullptr) : m_value(val)
        {}

        template <typename T>
        ValueT(const T& val,
               std::enable_if_t<std::is_enum<std::decay_t<T>>::value>* =nullptr) : m_value(static_cast<Enum>(val))
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

        bool isFirst() const noexcept
        {
            return isType(ValueType::FirstT);
        }

        bool isLast() const noexcept
        {
            return isType(ValueType::LastT);
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
        Error eachVectorIntervalItem(const HandlerT& handler) const
        {
            detail::VectorIntervalItemVisitor<HandlerT> v{handler};
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
        Error handleVectorInterval(const HandlerT& handler) const
        {
            detail::VectorIntervalVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error eachVectorItem(const HandlerT& handler)
        {
            detail::VectorItemVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename HandlerT>
        Error eachVectorIntervalItem(const HandlerT& handler)
        {
            detail::VectorIntervalItemVisitor<HandlerT> v{handler};
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

        template <typename HandlerT>
        Error handleVectorInterval(const HandlerT& handler)
        {
            detail::VectorIntervalVisitor<HandlerT> v{handler};
            return common::lib::variantVisit(v,m_value);
        }

        template <typename VisitorT>
        Error handleValue(VisitorT&& v) const
        {
            return common::lib::variantVisit(std::forward<VisitorT>(v),m_value);
        }

        template <typename T>
        const T& as() const
        {
            return common::lib::variantGet<T>(m_value);
        }

        const type& value() const
        {
            return m_value;
        }

    private:

        type m_value;
};

using ValueVariant=lib::variant<
    HATN_DB_QUERY_VALUE_TYPES(HATN_DB_QUERY_VALUE_TYPE)
>;

using Operand=ValueT<ValueVariant,ValueType>;

struct Field
{
    template <typename T>
    Field(
        const IndexFieldInfo* fieldInfo,
        Operator op,
        T&& value,
        Order order=Order::Asc
    ) : fieldInfo(fieldInfo),
        op(op),
        value(std::forward<T>(value)),
        order(order)
    {
        //! @todo Test vector of intervals
        static_assert(!hana::is_a<VectorIntervalTag,T>,"Vector of intervals is not properly supported yet");

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

        if (value.isFirst())
        {
            Assert(op==Operator::gte || op==Operator::eq,"Invalid operator for First operand, only eq and gte operators supported");
        }
        if (value.isLast())
        {
            Assert(op==Operator::lte || op==Operator::eq,"Invalid operator for Last operand, only eq and lte operators supported");
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

template <typename VecT>
void sortAndMerge(VecT& vec, Order order)
{
    using itemType=typename VecT::value_type;

    // sort vector
    std::sort(
        vec.begin(),
        vec.end(),
        std::less<itemType>{}
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

struct valueToDateRangeT
{
    common::DateRange operator()(const common::DateRange& range, DatePartitionMode) const
    {
        return range;
    }

    common::DateRange operator()(const common::DateTime& dt, DatePartitionMode mode) const
    {
        return common::DateRange{dt,mode};
    }

    common::DateRange operator()(const common::Date& dt, DatePartitionMode mode) const
    {
        return common::DateRange{dt,mode};
    }

    common::DateRange operator()(const ObjectId& id, DatePartitionMode mode) const
    {
        return id.toDateRange(mode);
    }

    template <typename T>
    common::DateRange operator()(const T&, DatePartitionMode) const
    {
        Assert(false,"Invalid type for conversion to DateRange");
        return common::DateRange{};
    }
};
constexpr valueToDateRangeT toDateRange{};

template <typename T, typename = hana::when<true>>
struct IsVector : public hana::false_
{
};

template <typename T>
struct IsVector<T,hana::when_valid<typename std::decay_t<T>::value_type>>
    : public hana::true_
{
};

template <typename FieldT, typename ValueT>
auto condition(const FieldT& field, Operator op, ValueT&& value, Order order=Order::Asc)
{
    static const auto isLval=std::is_lvalue_reference<ValueT>{};
    static const auto isStrView=
        hana::bool_c<
            std::is_same<lib::string_view,std::decay_t<ValueT>>::value
            ||
            std::is_same<const char*,ValueT>::value
            >;
    using isVecT=IsVector<ValueT>;

    static_assert(
        decltype(isLval)::value || decltype(isStrView)::value || !isVecT::value,
        "Do not use temporary/rvalue strings or vectors as a query field value"
    );

    if constexpr (std::is_convertible<ValueT,String>::value)
    {
        return hana::make_tuple(std::cref(field),op,String(value),order);
    }
    else
    {
        if constexpr (isVecT::value)
        {
            return hana::make_tuple(std::cref(field),op,std::cref(value),order);
        }
        else
        {
            return hana::make_tuple(std::cref(field),op,std::forward<ValueT>(value),order);
        }
    }
}

template <typename Ts>
struct whereT
{
    template <typename FieldT, typename ValueT>
    auto and_(const FieldT& field, Operator op, ValueT&& value, Order order=Order::Asc) &&
    {
        auto make=[&]()
        {
            return hana::append(std::move(conditions),condition(field,op,std::forward<ValueT>(value),order));
        };
        return whereT<decltype(make())>{make()};
    }

    template <typename Ys>
    whereT(Ys&& cond) : conditions(std::forward<Ys>(cond))
    {}

    constexpr static size_t size() noexcept
    {
        using count=decltype(hana::size(conditions));
        return count::value;
    }

    Ts conditions;
};

//! @note Vectors and strings must be used only by references escept for string_vire strings.
//! @note Vectors must be sorted, otherwise they will be sorted internally despite const lvalue reference.
template <typename FieldT, typename ValueT>
auto where(const FieldT& field, Operator op, ValueT&& value, Order order=Order::Asc)
{
    auto make=[&]()
    {
        return hana::make_tuple(condition(field,op,std::forward<ValueT>(value),order));
    };
    return whereT<decltype(make())>{make()};
}

} // namespace query

HATN_DB_NAMESPACE_END

#endif // HATNDBQUERY_H
