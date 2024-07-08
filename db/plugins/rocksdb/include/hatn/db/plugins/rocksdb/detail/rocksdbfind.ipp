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
struct valueToBuf
{
    BufT& buf;

    //! @todo implement operators
};

template <typename BufT, typename KeyHandlerT>
Error eachIndexKeyFieldItem(
        detail::FindCursor<BufT>& cursor,
        RocksdbHandler& handler,
        const IndexQuery& idxQuery,
        const KeyHandlerT& keyCallback,
        ROCKSDB_NAMESPACE::Snapshot* snapshot,
        size_t pos,
        const query::Field& field,
        AllocatorFactory* allocatorFactory
    )
{
    size_t count=0;

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
            fromBuf.append(NullCharStr);
            valueToBuf(fromBuf,field);
            fromBuf.append(OneCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::gte):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(NullCharStr);
            valueToBuf(fromBuf,field);
            fromBuf.append(NullCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::lt):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(NullCharStr);
            valueToBuf(toBuf,field);
            toBuf.append(NullCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::lte):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(NullCharStr);
            valueToBuf(toBuf,field);
            toBuf.append(OneCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::eq):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(NullCharStr);
            valueToBuf(fromBuf,field);
            fromBuf.append(NullCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(NullCharStr);
            valueToBuf(toBuf,field);
            toBuf.append(OneCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::in):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(NullCharStr);
            valueToBuf(fromBuf,field,false);
            if (field.isIntervalLeftOpen())
            {
                fromBuf.append(OneCharStr);
            }
            else
            {
                fromBuf.append(NullCharStr);
            }
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(NullCharStr);
            valueToBuf(toBuf,field,true);
            if (field.isIntervalRightOpen())
            {
                toBuf.append(NullCharStr);
            }
            else
            {
                toBuf.append(OneCharStr);
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
    while (it.Valid())
    {
        // check if value expired
        const auto keyValue=it.Slice();
        //! @todo Filter key by timestamp range
        if (!TtlMark::isExpired(keyValue))
        {
            // construct key prefix
            auto key=it.Key();
            auto currentKey=detail::keyPrefix(key,pos);
            cursor.resetPartial(currentKey,pos);

            // append final keys to result
            if (pos==query.fields().size())
            {
                keyCallback(key);
                // result.emplace_back(common::pmr::string{key.data(),key.size()});
            }
            else
            {
                auto ec=nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot);
                if (ec)
                {
                    return ec;
                }
            }

            // check limit
            if ((idxQuery.limit()>0) && (count==idxQuery.limit()))
            {
                break;
            }
        }

        // get the next/prev key
        if (iterateForward)
        {
            it.Next();
        }
        else
        {
            it.Prev();
        }
    }
    // check iterator status
    if (!it.status().ok())
    {
        HATN_CTX_SCOPE_ERROR("index-find")
        return makeError(DbError::READ_FAILED,it.status());
    }

    // done
    return OK;
}

template <typename BufT, typename KeyHandlerT>
Error nextKeyField(
                   detail::FindCursor<BufT>& cursor,
                   RocksdbHandler& handler,
                   const IndexQuery& query,
                   const KeyHandlerT& keyCallback,
                   ROCKSDB_NAMESPACE::Snapshot* snapshot
                )
{
    // check if all fields procesed
    if (cursor.pos==query.fields().size())
    {
        return OK;
    }

    // glue key fields if operators and orders match
    const auto& queryField=query.field(cursor.pos);
    if (cursor.pos==0 ||
            query.field(cursor.pos-1).matchOp(queryField)
        )
    {
        // append field to cursor
        cursor.partialKey.append(
                fieldToStringBuf(detail::PartialFindKey.buf,queryField.value()())
            );
        ++cursor.pos;

        // go to next field
        if (cursor.pos<query.fields().size())
        {
            return nextKeyField(cursor,result,handler,query,keyCallback,snapshot);
        }
    }
    else
    {
        ++cursor.pos;
    }

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
