/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbfind.ipp
  *
  *   RocksDB database template for finding objects.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFIND_IPP
#define HATNROCKSDBFIND_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace query
{

struct Null{};

enum class Operator : uint8_t
{
    any,
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

class Value
{
    public:

        using type=lib::variant<
            Null,
            bool,
            int8_t,
            int16_t,
            int32_t,
            int64_t,
            uint8_t,
            uint16_t,
            uint32_t,
            uint64_t,
            // float,
            // double,
            common::pmr::string,
            common::DateTime,
            common::Date,
            common::Time,
            common::DateRange,
            ObjectId
            >;

        enum class Type : uint8_t
        {
            Null,
            Bool,
            Int8,
            Int16,
            Int32,
            Int64,
            Uint8,
            Uint16,
            Uint32,
            Uint64,
            // Float,
            // Double,
            String,
            DateTime,
            Date,
            Time,
            DateRange,
            ObjectId
        };

        template <typename T>
        Operand(T&& val) : m_value(std::forward<T>(val))
        {}

        Type type() const noexcept
        {
            return static_cast<Type>(lib::variantIndex(m_value));
        }

        bool isType(Type t) const noexcept
        {
            return type()==t;
        }

        type& operator()() noexcept
        {
            return m_value;
        }

        const type& operator()() const noexcept
        {
            return m_value;
        }

    private:

        type m_value;
};

struct Interval
{
    enum class Type : uint8_t
    {
        Closed,
        Open
    };

    struct Endpoint
    {
        Value value;
        Type type;
    };

    Endpoint from;
    Endpoint to;
};

using Vector=common::pmr::vector<Value>;

class Operand
{
    public:

        using type=lib::variant<
            Value,
            Interval,
            Vector
        >;

        enum class Type : uint8_t
        {
            Value,
            Interval,
            Vector
        };

        template <typename T>
        Operand(T&& val) : m_value(std::forward<T>(val))
        {}

        Type type() const noexcept
        {
            return static_cast<Type>(lib::variantIndex(m_value));
        }

        bool isType(Type t) const noexcept
        {
            return type()==t;
        }

        type& operator()() noexcept
        {
            return m_value;
        }

        const type& operator()() const noexcept
        {
            return m_value;
        }

    private:

        type m_value;
};

struct Field
{
    template <typename T>
    Field(Operator op, T&& value, Order order=Order::Asc)
        : op(op),
          value(std::forward<T>(value)),
          order(order)
    {}

    Operator op;
    Operand value;
    Order order;
};

} // namespace query

struct IndexQuery
{
    IndexQuery(uint32_t index,
            const common::pmr::polymorphic_allocator<query::Field>& allocator
        ) : index(index),
            fields(allocator)
    {}

    IndexQuery(uint32_t index,
            std::initializer_list<query::Field> list,
            const common::pmr::polymorphic_allocator<query::Field>& allocator
        ) : index(index),
            fields(std::move(list),allocator)
    {}

    IndexQuery(uint32_t index,
            AllocatorFactory* allocatorFactory=AllocatorFactory::getDefault()
        ) : index(index),
            fields(allocatorFactory->dataAllocator<query::Field>())
    {}

    IndexQuery(uint32_t index,
            std::initializer_list<query::Field> list,
            AllocatorFactory* allocatorFactory=AllocatorFactory::getDefault()
        ) : index(index),
            fields(std::move(list),allocatorFactory->dataAllocator<query::Field>())
    {}

    uint32_t index;
    common::pmr::vector<query::Field> fields;
};

struct FindT
{
    template <typename ModelT>
    Result<common::pmr::vector<UnitWrapper>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Namespace& ns,
        const IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const;
};
constexpr FindT Find{};

template <typename ModelT>
Result<common::pmr::vector<UnitWrapper>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Namespace& ns,
        const IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbfind")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())

    return Error{CommonError::NOT_IMPLEMENTED};
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIND_IPP
