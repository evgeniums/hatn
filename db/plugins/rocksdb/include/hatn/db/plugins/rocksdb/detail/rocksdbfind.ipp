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
#include <hatn/db/index.h>
#include <hatn/db/model.h>

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
    Field(const IndexFieldInfo& fieldInfo, Operator op, T&& value, Order order=Order::Asc)
        : fieldInfo(fieldInfo),
          op(op),
          value(std::forward<T>(value)),
          order(order)
    {
        checkOperator();
    }

    void checkOperator() const
    {
        bool ok=true;
        if (value.isType(Operand::Type::Interval) || value.isType(Operand::Type::Vector))
        {
            ok=op==Operator::in || op==Operator::nin;
        }
        else
        {
            ok=op!=Operator::in && op!=Operator::nin;
        }
        Assert(ok,"Invalid combination of operator and operand");
    }

    bool match(const Field& other, bool excludeInNin=true) const noexcept
    {
        auto ok=op==other.op && order==other.order;
        if (excludeInNin)
        {
            return ok && !operatorInOrNin();
        }
        return ok;
    }

    bool operatorInOrNin() const noexcept
    {
        return op==Operator::in || op==Operator::nin;
    }

    const IndexFieldInfo& fieldInfo;
    Operator op;
    Operand value;
    Order order;
};

} // namespace query

class IndexQuery
{
    public:

        IndexQuery(const IndexInfo& index,
                size_t limit,
                const common::pmr::polymorphic_allocator<query::Field>& allocator
            ) : m_index(index),
                m_fields(allocator),
                m_limit(limit)
        {}

        IndexQuery(const IndexInfo& index,
                std::initializer_list<query::Field> list,
                size_t limit,
                const common::pmr::polymorphic_allocator<query::Field>& allocator
            ) : m_index(index),
                m_fields(std::move(list),allocator),
                m_limit(limit)
        {
            checkFields();
        }

        IndexQuery(const IndexInfo& index,
                size_t limit,
                AllocatorFactory* allocatorFactory=AllocatorFactory::getDefault()
            ) : m_index(index),
                m_fields(allocatorFactory->dataAllocator<query::Field>()),
                m_limit(limit)
        {}

        IndexQuery(const IndexInfo& index,
                std::initializer_list<query::Field> list,
                size_t limit,
                AllocatorFactory* allocatorFactory=AllocatorFactory::getDefault()
            ) : m_index(index),
                m_fields(std::move(list),allocatorFactory->dataAllocator<query::Field>()),
                m_limit(limit)
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
            return m_index;
        }

        const auto& fields() const noexcept
        {
            return m_fields;
        }

        const auto& field(size_t pos) const noexcept
        {
            return m_fields[pos];
        }

        size_t limit() const noexcept
        {
            return m_limit;
        }

    private:

        void checkFields() const
        {
            Assert(m_fields.size()<=m_index.fields().size(),"Number of fields must be less or equal to number of fields in the index");

            for (size_t i=0;i<m_fields.size();i++)
            {
                const auto& qF=m_fields[i];
                const auto& iF=m_index.fields()[i];
                if (iF.nested())
                {
                    Assert(iF.name()==qF.fieldInfo.name(),"Field name mismatches name of corresponding index field");
                }
                else
                {
                    Assert(iF.id()==qF.id,"Field ID mismatches ID of corresponding index field");
                }
            }
        }

        const IndexInfo& m_index;
        common::pmr::vector<query::Field> m_fields;
        size_t m_limit;
};

template <typename BufT>
struct FindT
{
    template <typename ModelT>
    Result<common::pmr::vector<UnitWrapper>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Topics& topics,
        const IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const;
};
template <typename BufT>
constexpr FindT<BufT> Find{};

namespace detail {

template <typename BufT>
struct PartialFindKey
{
    PartialFindKey(
            size_t fieldCount,
            AllocatorFactory* allocatorFactory
        ) : buf(allocatorFactory->bytesAllocator()),
            offsets(allocatorFactory->dataAllocator())
    {
        offsets.reserve(fieldCount);
    }

    template <typename T>
    void append(const T& str)
    {
        offsets.push_back(buf.size());
        if (!buf.empty())
        {
            buf.append(NullCharStr);
        }
        buf.append(str);
    }

    void pop(size_t n=1)
    {
        size_t pos=offsets.size()-n;
        size_t offset=offsets[pos];
        buf.resize(offset);
        offsets.resize(pos);
    }

    void reset()
    {
        buf.reset();
        offsets.clear();
    }

    BufT buf;
    common::pmr::vector<uint32_t> offsets;
};

using FindKeys=common::pmr::vector<common::pmr::string>;
using TopicKeys=common::pmr::vector<FindKeys>;

template <typename BufT>
struct FindCursor
{
    FindCursor(
            const lib::string_view& indexId,
            const lib::string_view& topic,
            std::shared_ptr<RocksdbPartition> partition,
            common::pmr::AllocatorFactory* allocatorfactory
        ) : partialKey(allocatorfactory->bytesAllocator()),
            pos(0),
            partition(std::move(partition))
    {
        partialKey.append(topic);
        partialKey.append(NullCharStr);
        partialKey.append(indexId);
        partialKey.append(NullCharStr);
    }

    void resetPartial(const lib::string_view& prefixKey, size_t p)
    {
        partialKey.clear();
        partialKey.append(prefixKey);
        pos=p;
    }

    BufT partialKey;
    size_t pos;

    std::shared_ptr<RocksdbPartition> partition;
};

lib::string_view keyPrefix(const lib::string_view& key, size_t pos) const noexcept
{
    //! @todo Implement extraction of key prefix
    return key;
}

using PartitionKeys=std::pair<std::shared_ptr<RocksdbPartition>,
                              TopicKeys
                             >;

using PartitionsKeys=common::pmr::vector<PartitionKeys>;

} // namespace detail


template <typename BufT>
Error nextKeyField(
                   detail::FindCursor<BufT>& cursor,
                   detail::FindKeys& result,
                   RocksdbHandler& handler,
                   const IndexQuery& query
                )
{
    // check if all fields procesed
    if (cursor.pos==query.fields().size())
    {
        return OK;
    }

    // glue key fields if operands and orders match
    const auto& queryField=query.field(cursor.pos);
    if (cursor.pos==0 ||
            query.field(cursor.pos-1).match(queryField)
        )
    {
        //! @todo append field to cursor
        cursor.partialKey.append(
                fieldToStringBuf(detail::PartialFindKey.buf,queryField.value()())
            );
        ++cursor.pos;

        // go to next field
        if (cursor.pos<query.fields().size())
        {
            return nextKeyField(cursor,result,handler,query);
        }
    }
    else
    {
        ++cursor.pos;
    }

    //! @todo read keys with partial query for current field
    ROCKSDB_NAMESPACE::Iterator it;
    // ...
    size_t pos=cursor.pos;
    while (it.Valid())
    {
        // check if value expired
        const auto keyValue=it.Slice();
        if (!TtlMark::isExpired(keyValue))
        {
            // construct key prefix
            auto key=it.Key();
            auto currentKey=detail::keyPrefix(key,pos);
            cursor.resetPartial(currentKey,pos);

            // append final keys to result
            if (pos==query.fields().size())
            {
                result.emplace_back(common::pmr::string{key.data(),key.size()});
            }
            else
            {
                auto ec=nextKeyField(cursor,result,handler,query);
                if (ec)
                {
                    //! @todo Log error
                    return ec;
                }
            }

            // check limit
            if ((query.limit()>0) && (result.size()==query.limit()))
            {
                break;
            }
        }

        // get the next/prev key
        if (queryField.order==query::Order::Desc)
        {
            it.Prev();
        }
        else
        {
            it.Next();
        }
    }
    //! @todo check iterator status

    // done
    return OK;
}

template <typename BufT>
template <typename ModelT>
Result<common::pmr::vector<UnitWrapper>> FindT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const Topics& topics,
        const IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbfind")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())
    HATN_CTX_SCOPE_PUSH("topic",ns.topic())

    // figure out partitions for processing
    common::pmr::vector<detail::FindPartitions> partitions{1,allocatorFactory->dataAllocator()};
    const auto& field0=query.field(i);
    hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            if (modelType::isDatePartitionField(_(field0).fieldInfo.name()))
            {
                //! @todo collect partitions matching query expression for the first field
            }
            else
            {
                //! @todo use all partitions
            }
        },
        [&](auto _)
        {
            _(partitions)[0]=std::make_pair<detail::FindPartitions>(
                _(handler).defaultPartition(),
                detail::FindKeys{allocatorFactory->dataAllocator()}
                );
        }
    );

    // process all partitions
    detail::PartitionsKeys partitionsKeys{allocatorFactory->dataAllocator()};
    partitionsKeys.reserve(partitions.size());
    for (auto&& partition: partitions)
    {
        auto& partitionKeys=partitionsKeys.emplace_back(
            std::make_pair<std::shared_ptr<RocksdbPartition>,
                           detail::TopicKeys
                          >(
                                partition,
                                allocatorFactory->dataAllocator()
                            )
        );
        partitionKeys.second.reserve(topics.size());

        // process all topics topic
        for (auto&& topic: topics)
        {
            auto& keys=partitionKeys.second.emplace_back(allocatorFactory->dataAllocator());
            detail::FindCursor<BufT> cursor(query.index().id(),topic,partition,allocatorFactory);
            auto ec=nextKeyField(cursor,keys,handler,query);
            if (ec)
            {
                //! @todo log error
                return ec;
            }
        }
    }

    //! @todo merge keys

    //! @todo get objects

    // done
    return Error{CommonError::NOT_IMPLEMENTED};
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIND_IPP
