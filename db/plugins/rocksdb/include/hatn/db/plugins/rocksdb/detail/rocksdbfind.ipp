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
#include <hatn/db/plugins/rocksdb/detail/indexkeysearch.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT>
struct FindT
{
    template <typename ModelT>
    Result<common::pmr::vector<UnitWrapper>> operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const IndexQuery& query,
        AllocatorFactory* allocatorFactory
    ) const;
};
template <typename BufT>
constexpr FindT<BufT> Find{};

namespace detail {

struct IndexKey
{
    IndexKey(
            ROCKSDB_NAMESPACE::Slice* k,
            ROCKSDB_NAMESPACE::Slice* v,
            RocksdbPartition* p,
            AllocatorFactory* allocatorFactory
        ) : key(k->data(),k->size(),allocatorFactory->bytesAllocator()),
            value(v->data(),v->size(),allocatorFactory->bytesAllocator()),
            partition(p),
            keyParts(allocatorFactory->dataAllocator<lib::string_view>())
    {
        fillKeyParts();
    }

    void fillKeyParts()
    {
        //! @todo split key to key parts
    }

    common::pmr::string key;
    common::pmr::string value;
    RocksdbPartition* partition;

    common::pmr::vector<lib::string_view> keyParts;
};

struct IndexKeyCompare
{
    IndexKeyCompare(const IndexQuery& idxQuery) : idxQuery(&idxQuery)
    {}

    operator bool()(const IndexKey& l, const IndexKey& r) const noexcept
    {
        //! @todo compare key parts according to ordering of query fields
        return lib::string_view{l.key.data(),l.key.size()}<lib::string_view{r.key.data(),r.key.size()};
    }

    const IndexQuery* idxQuery;
};

using IndexKeys=common::pmr::FlatSet<IndexKey,IndexKeyCompare>;

} // namespace detail

template <typename BufT>
template <typename ModelT>
Result<common::pmr::vector<UnitWrapper>> FindT<BufT>::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const IndexQuery& idxQuery,
        AllocatorFactory* allocatorFactory
    ) const
{
    using modelType=std::decay_t<ModelT>;

    HATN_CTX_SCOPE("rocksdbfind")
    HATN_CTX_SCOPE_PUSH("coll",model.collection())

    // figure out partitions for processing
    common::pmr::vector<std::shared_ptr<RocksdbPartition>> partitions{1,allocatorFactory->dataAllocator()};
    const auto& field0=idxQuery.field(0);
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
            _(partitions)[0]=_(handler).defaultPartition();
        }
    );

    // collect matching keys ordered according to index query
#if 0
    size_t topicKeys=0;
#endif
    detail::IndexKeys indexKeys{allocatorFactory->dataAllocator(),detail::IndexKeyCompare{idxQuery}};
    auto keyCallback=[&indexKeys,&idxQuery,allocatorFactory](RocksdbPartition* partition,
                                                          ROCKSDB_NAMESPACE::Slice* key,
                                                          ROCKSDB_NAMESPACE::Slice* keyValue,
                                                          Error&
                                                    )
    {
        //! @todo optimization: append presorted keys in case of first topic of first partition
        //! or in case of first topic of each partition if partitions are ordered

        // insert found key
        auto it=indexKeys.insert(detail::IndexKey{key,keyValue,partition,allocatorFactory});
        auto insertedIdx=it.first.index();

        // cut keys number to limit
        if (idxQuery.limit()!=0 && indexKeys.size()>idxQuery.limit())
        {
            indexKeys.resize(idxQuery.limit());

            // if inserted key was dropped over the limit then break current iteration because keys are pre-sorted
            // and all next keys will be dropped anyway
            if (insertedIdx==idxQuery.limit())
            {
                return false;
            }
        }

#if 0
        //! @todo Check if it is needed. Seems like the condition above will cover the case below.
        // limit number of iterations for each topic
        ++topicKeys;
        return (idxQuery.limit()==0) || (topicKeys<idxQuery.limit());
#else
        return true;
#endif
    };

    // get rocksdb snapshot
    ROCKSDB_NAMESPACE::ManagedSnapshot managedSnapchot{handler.p()->db};
    const auto* snapshot=managedSnapchot.snapshot();

    // process all partitions
    //! @todo if query starts with partition field then pre-sort partitons by order of that field
    for (const auto& partition: partitions)
    {
        // process all topics
        for (const auto& topic: idxQuery.topics())
        {
#if 0
            topicKeys=0;
#endif
            index_key_search::Cursor<BufT> cursor(idxQuery.index().id(),topic,partition.get(),allocatorFactory);
            auto ec=index_key_search::nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
            if (ec)
            {
                //! @todo log error
                return ec;
            }
        }

        //! @todo if query starts with partition field then break if limit reached
    }

    // prepare result
    common::pmr::vector<UnitWrapper> objects{allocatorFactory->dataAllocator<UnitWrapper>()};

    // if keys not found then return empty result
    if (indexKeys.empty())
    {
        return objects;
    }

    // fill result
    ROCKSDB_NAMESPACE::ReadOptions readOptions=handler.p()->readOptions;
    readOptions.snapshot=snapshot;
    objects.reserve(indexKeys.size());
    for (auto&& key:indexKeys)
    {
        //! @todo get object from rockdb

        //! @todo create unit

        //! @todo deserialize object

        //! @todo push wrapped unit to result vector
    }

    // done
    return objects;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIND_IPP
