/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbmodelt.ipp
  *
  *   RocksDB database model template.
  *
  */

/****************************************************************************/

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>

HATN_DB_USING

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbModelT **************************/

template <typename ModelT>
std::multimap<FieldPath,UpdateIndexKeyExtractor<ModelT>,FieldPathCompare> RocksdbModelT<ModelT>::updateIndexKeyExtractors;

template <typename ModelT>
common::FlatSet<FieldPath,FieldPathCompare> RocksdbModelT<ModelT>::ttlFields;

//---------------------------------------------------------------

template <typename ModelT>
template <typename T>
void RocksdbModelT<ModelT>::init(const T& model)
{
    // The extractors are per C++ model type, but one type can have several model instances (e.g.
    // one cache model per collection) and a model instance can be destroyed and built again (a
    // new app in the same process). So they are filled once, and keep only the position of each
    // index: the index itself is taken from the model the update runs against. Capturing &idx
    // here kept a reference into the first model registered, read after it was freed, and also
    // added a set of extractors for every model instance of the type, so an update of one
    // collection computed keys for the indexes of the others.
    static std::once_flag once;
    std::call_once(once,[&model]()
    {
        auto eachIndex=[&model](auto pos)
        {
            const auto& idx=hana::at(model.indexes,pos);
            if constexpr (!std::decay_t<decltype(idx)>::isDatePartitioned())
            {
                auto eachField=[&idx,pos](const auto& field)
                {
                    auto handler=[pos](
                                       const ModelT& m,
                                       Keys& keysHandler,
                                       const lib::string_view& topic,
                                       const ROCKSDB_NAMESPACE::Slice& objectId,
                                       const ObjectT* obj,
                                       IndexKeyUpdateSet& keys
                                       )
                    {
                        const auto& index=hana::at(m.indexes,pos);
                        std::ignore=keysHandler.makeIndexKey(topic,objectId,obj,index,
                                                               [&keys,&index](auto&& key, Keys::IsIndexSet isIndexSet)
                                                               {
                                                                   keys.insert(IndexKeyUpdate{index.name(),key,index.unique(),isIndexSet==Keys::IsIndexSet::Yes});
                                                                   return Error{OK};
                                                               }
                                                               );
                    };
                    updateIndexKeyExtractors.insert(std::make_pair(fieldPath(field),handler));
                    if (idx.isTtl())
                    {
                        ttlFields.insert(fieldPath(field));
                    }
                };
                hana::for_each(idx.fields,eachField);
            }
        };
        hana::for_each(hana::make_range(hana::size_c<0>,hana::size(model.indexes)),eachIndex);
    });
}

//---------------------------------------------------------------

template <typename ModelT>
void RocksdbModelT<ModelT>::updatingKeys(
        const ModelT& model,
        Keys& keysHandler,
        const update::Request& request,
        const lib::string_view& topic,
        const ROCKSDB_NAMESPACE::Slice& objectId,
        const ObjectT* object,
        IndexKeyUpdateSet& keys,
        bool ttlUpdated
    )
{
    for (auto&& field : request)
    {
        if (ttlUpdated)
        {
            for (auto&& it:updateIndexKeyExtractors)
            {
                it.second(model,keysHandler,topic,objectId,object,keys);
            }
        }
        else
        {
            auto range=updateIndexKeyExtractors.equal_range(field.path);
            for (auto it=range.first;it!=range.second;++it)
            {
                (it->second)(model,keysHandler,topic,objectId,object,keys);
            }
        }
    }
}

//---------------------------------------------------------------

template <typename ModelT>
bool RocksdbModelT<ModelT>::checkTtlFieldUpdated(const update::Request& request) noexcept
{
    static FieldPath pathOfUpdatedAt{makePath(object::updated_at)};

    if (ttlFields.find(pathOfUpdatedAt)!=ttlFields.end())
    {
        return true;
    }

    for (auto&& field : request)
    {
        if (ttlFields.find(field.path)!=ttlFields.end())
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
