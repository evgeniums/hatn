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
        auto objectIdS=KeysBase::objectIdFromIndexValue(val->data(),val->size());

        // update
        auto r=updateSingle(keys,objectIdS,*key,model,handler,partition,topic,request,modifyReturnFirst,allocatorFactory,tx);
        if (r)
        {
            ec=std::move(r.takeError());
            return false;
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
