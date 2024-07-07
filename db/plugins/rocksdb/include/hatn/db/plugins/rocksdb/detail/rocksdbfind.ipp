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

    // glue key fields if operators and orders match
    const auto& queryField=query.field(cursor.pos);
    if (cursor.pos==0 ||
            query.field(cursor.pos-1).matchOp(queryField)
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


//! @todo Split in/nin/neq queries to sets of queries

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
