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
std::multimap<std::string,UpdateIndexKeyExtractor<typename ModelT::Type>> RocksdbModelT<ModelT>::updateIndexKeyExtractors;

template <typename ModelT>
common::FlatSet<std::string> RocksdbModelT<ModelT>::ttlFields;

inline static std::string _updatedAtFieldName{"updated_at"};

//---------------------------------------------------------------

template <typename ModelT>
template <typename T>
void RocksdbModelT<ModelT>::init(const T& model)
{
    auto eachIndex=[](const auto& idx)
    {
        auto eachField=[&idx](const auto& field)
        {
            auto handler=[&idx](
                      Keys& keysHandler,
                      const lib::string_view& topic,
                      const ROCKSDB_NAMESPACE::Slice& objectId,
                      const ObjectT* obj,
                      IndexKeyUpdateSet& keys
                    )
            {
                keysHandler.makeIndexKey(topic,objectId,obj,idx,
                    [&keys,&idx](auto&& key)
                    {
                        IndexKeyUpdate k{key};
                        k.unique=idx.unique();
                        keys.insert(std::move(k));
                        return Error{OK};
                    }
                );
            };
            updateIndexKeyExtractors.insert(std::make_pair(field.name(),handler));
        };
        hana::for_each(idx.fields,eachField);
    };
    hana::for_each(model.indexes,eachIndex);
}

//---------------------------------------------------------------

template <typename ModelT>
void RocksdbModelT<ModelT>::updatingKeys(
        Keys& keysHandler,
        const update::Request& request,
        const lib::string_view& topic,
        const ROCKSDB_NAMESPACE::Slice& objectId,
        const ObjectT* object,
        IndexKeyUpdateSet& keys
    )
{
    for (auto&& field : request)
    {
        auto range=updateIndexKeyExtractors.equal_range(field.fieldInfo->name());
        for (auto i=range.first;i!=range.second;++i)
        {
            (i->second)(keysHandler,topic,objectId,object,keys);
        }
    }
}

//---------------------------------------------------------------

template <typename ModelT>
bool RocksdbModelT<ModelT>::checkTtlFieldUpdated(const update::Request& request) noexcept
{
    if (ttlFields.find(_updatedAtFieldName)!=ttlFields.end())
    {
        return true;
    }

    for (auto&& field : request)
    {
        if (ttlFields.find(field.fieldInfo->name())!=ttlFields.end())
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
