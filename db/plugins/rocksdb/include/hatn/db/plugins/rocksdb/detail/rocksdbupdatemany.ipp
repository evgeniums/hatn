/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbupdate.ipp
  *
  *   RocksDB database template for updating many objects.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBUPDATEMANY_IPP
#define HATNROCKSDBUPDATEMANY_IPP

#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbupdate.ipp>
#include <hatn/db/plugins/rocksdb/detail/findmodifymany.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct UpdateManyT
{
    template <typename ModelT>
    Result<typename ModelT::SharedPtr> operator ()(const ModelT& model,
                                                  RocksdbHandler& handler,
                                                  const ModelIndexQuery& query,
                                                  const update::Request& request,
                                                  db::update::ModifyReturn modifyReturnFirst,
                                                  AllocatorFactory* allocatorFactory,
                                                  Transaction* tx,
                                                  bool single=false
                                                  ) const;
};
constexpr UpdateManyT UpdateMany{};

template <typename ModelT>
Result<typename ModelT::SharedPtr> UpdateManyT::operator ()(
        const ModelT& model,
        RocksdbHandler& handler,
        const ModelIndexQuery& query,
        const update::Request& request,
        db::update::ModifyReturn modifyReturnFirst,
        AllocatorFactory* allocatorFactory,
        Transaction* tx,
        bool single
    ) const
{
    HATN_CTX_SCOPE("updatemany")
    Keys keys{allocatorFactory};
    typename ModelT::SharedPtr result;

    size_t i=0;
    auto keyCallback=[&](RocksdbPartition* partition,
                                                      const lib::string_view& topic,
                                                      ROCKSDB_NAMESPACE::Slice* key,
                                                      ROCKSDB_NAMESPACE::Slice* val,
                                                      Error& ec
                                                      )
    {
        // construct object ID
        auto objectIdS=Keys::objectIdFromIndexValue(val->data(),val->size());
        auto objectKey=Keys::objectKeyFromIndexValue(val->data(),val->size());

        // update
        bool brokenIdx=false;
        auto r=updateSingle(keys,objectIdS,objectKey,model,handler,partition,topic,request,modifyReturnFirst,allocatorFactory,tx,&brokenIdx);
        if (r)
        {
            ec=std::move(r.takeError());
            return false;
        }
        if (brokenIdx)
        {
            HATN_CTX_WARN_RECORDS("missing object in rocksdb",
                                  {"idx_key",lib::toStringView(logKey(*key))},
                                  {"obj_key",lib::toStringView(logKey(objectKey))},
                                  {"obj_id",sliceView(objectIdS)},
                                  {"db_partition",partition->range}
                                  )
        }

        // fill return object with first found
        if (modifyReturnFirst!=db::update::ModifyReturn::None && i==0)
        {
            result=r.takeValue();
        }
        i++;

        if (single)
        {
            return false;
        }

        // done
        return true;
    };

    // iterate
    auto ec=FindModifyMany(model,handler,query,allocatorFactory,keyCallback);
    HATN_CHECK_EC(ec)

    // done
    return result;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUPDATEMANY_IPP
