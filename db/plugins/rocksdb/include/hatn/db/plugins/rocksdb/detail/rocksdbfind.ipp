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
#include <hatn/db/query.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

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
            buf.append(SeparatorCharStr);
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
        partialKey.append(SeparatorCharStr);
        partialKey.append(indexId);
        partialKey.append(SeparatorCharStr);
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

template <typename BufT, typename EndpointT=hana::true_>
struct valueVisitor
{
    constexpr static const EndpointT endpoint{};

    template <typename T>
    void operator()(const query::Interval<T>& value) const
    {
        hana::if_(
            endpoint,
            fieldToStringBuf(buf,value.from),
            fieldToStringBuf(buf,value.to)
        );
        buf.append(sep);
    }

    template <typename T>
    void operator()(const T& value) const
    {
        fieldToStringBuf(buf,value);
        buf.append(sep);
    }

    void operator()(const query::Last&) const
    {
        // replace previous separator with SeparatorCharPlusStr
        buf[buf.size()-1]=SeparatorCharPlusStr[0];
    }

    void operator()(const query::First&) const
    {}

    valueVisitor(BufT& buf, const lib::string_view& sep):buf(buf),sep(sep)
    {}

    BufT& buf;
    const lib::string_view& sep;
};

template <typename EndpointT=hana::true_>
struct fieldValueToBufT
{
    template <typename BufT>
    void operator()(BufT& buf, const query::Field& field, const lib::string_view& sep) const
    {
        lib::variantVisit(valueVisitor<BufT,EndpointT>{buf,sep},field.value());
    }
};
constexpr fieldValueToBufT<hana::true_> fieldValueToBuf{};

bool indexFiltered(const IndexQuery& idxQuery, size_t pos, const ROCKSDB_NAMESPACE::Slice* key, const ROCKSDB_NAMESPACE::Slice* value)
{
    //! @todo Implement index filtering
    return false;
}

template <typename BufT, typename KeyHandlerT>
Error eachIndexKeyFieldItem(
        detail::FindCursor<BufT>& cursor,
        RocksdbHandler& handler,
        const IndexQuery& idxQuery,
        const KeyHandlerT& keyCallback,
        ROCKSDB_NAMESPACE::Snapshot* snapshot,
        const query::Field& field,
        AllocatorFactory* allocatorFactory
    )
{
    size_t pos=cursor.pos;

    // prepare read options
    ROCKSDB_NAMESPACE::ReadOptions readOptions=handler.p()->readOptions;
    readOptions.snapshot=snapshot;
    bool iterateForward=field.order==query::Order::Asc;

    BufT fromBuf{allocatorFactory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice fromS;
    BufT toBuf{allocatorFactory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice toS;
    switch(field.op)
    {
        case (query::Operator::gt):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharPlusStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::gte):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::lt):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::lte):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::eq):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::in):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            if (field.value.fromIntervalType()==query::IntervalType::Open)
            {
                fieldValueToBuf(fromBuf,field,SeparatorCharPlusStr);
            }
            else
            {
                fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            }
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            constexpr static const fieldValueToBufT<hana::false_> toValueToBuf{};            
            if (field.value.toIntervalType()==query::IntervalType::Open)
            {
                toValueToBuf(toBuf,field,SeparatorCharStr);
            }
            else
            {
                toValueToBuf(toBuf,field,SeparatorCharPlusStr);
            }
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;

        default:
        {
            Assert(false,"Invalid operand");
        }
        break;
    }

    // create iterator
    std::unique_ptr<ROCKSDB_NAMESPACE::Iterator> it{handler.p()->db->NewIterator(readOptions,cursor.partition->indexCf.get())};

    // set start position of the iterator
    if (iterateForward)
    {
        it->SeekToFirst();
    }
    else
    {
        it->SeekToLast();
    }

    // iterate
    Error ec;
    while (it->Valid())
    {
        const auto* key=it->Key();
        const auto* keyValue=it->Slice();

        // check if key must be filtered
        if (!TtlMark::isExpired(keyValue)
            && !indexFiltered(idxQuery,pos,key,keyValue)
            )
        {
            // construct key prefix
            auto currentKey=detail::keyPrefix(key,pos);
            cursor.resetPartial(currentKey,pos);

            // call result callback for finally assembled key
            if (pos==idxQuery.fields().size())
            {
                if (!keyCallback(key,ec))
                {
                    return ec;
                }
            }
            else
            {
                auto ec=nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
                HATN_CHECK_EC(ec)
            }
        }

        // get the next/prev key
        if (iterateForward)
        {
            it->Next();
        }
        else
        {
            it->Prev();
        }
    }
    // check iterator status
    if (!it->status().ok())
    {
        HATN_CTX_SCOPE_ERROR("index-find")
        return makeError(DbError::READ_FAILED,it->status());
    }

    // done
    return OK;
}

template <typename BufT, typename KeyHandlerT>
Error nextKeyField(
                   detail::FindCursor<BufT>& cursor,
                   RocksdbHandler& handler,
                   const IndexQuery& idxQuery,
                   const KeyHandlerT& keyCallback,
                   ROCKSDB_NAMESPACE::Snapshot* snapshot,
                   AllocatorFactory* allocatorFactory
                )
{
    Assert(cursor.pos<idxQuery.fields().size(),"Out of range of cursor position of index query");

    // extract query field
    const auto& queryField=idxQuery.field(cursor.pos);
    ++cursor.pos;

    // glue scalar fields if operators and orders match
    if (cursor.pos<idxQuery.fields().size() && idxQuery.field(cursor.pos).matchScalarOp(queryField))
    {
        // append field to cursor
        cursor.partialKey.append(SeparatorCharStr);
        fieldValueToBuf(cursor.partialKey,queryField.value());

        // go to next field
        return nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
    }

    // iterate
    if ((queryField.value.isScalarType() || queryField.value.isIntervalType()) && queryField.op!=query::Operator::neq)
    {
        // process scalar or interval fields
        auto ec=eachIndexKeyFieldItem(cursor,
                                        idxQuery,
                                        handler,
                                        keyCallback,
                                        snapshot,
                                        queryField,
                                        allocatorFactory
                                    );
        HATN_CHECK_EC(ec)
    }
    else if (queryField.op==query::Operator::neq)
    {
        //! @todo process neq operator
        // split to lt and gt queries
    }
    else
    {
        //! @todo process vector
        // split to queries for each vector item
        // only in and nin operators supported
    }

    // done
    return OK;
}

} // namespace detail

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
