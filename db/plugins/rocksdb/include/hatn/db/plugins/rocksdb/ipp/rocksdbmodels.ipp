/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbmodels.ipp
  *
  *   Contains definition of RocksDB models singleton.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBMODELS_IPP
#define HATNROCKSDBMODELS_IPP

#include <boost/hana.hpp>

#include <hatn/common/sharedptr.h>

#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbreadobject.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbfind.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbfindcb.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdeletemany.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbupdate.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbupdatemany.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbcount.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

#include <hatn/db/plugins/rocksdb/rocksdbmodels.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ModelT>
void RocksdbModels::registerModel(std::shared_ptr<ModelWithInfo<ModelT>> model,
                                  const AllocatorFactory* allocatorFactory)
{
    Assert(m_models.find(model->info->modelId())==m_models.end(),"Failed to register duplicate model");

    auto rdbModel=std::make_shared<RocksdbModel>(model->info);

    using modelT=typename std::decay_t<decltype(model)>::element_type;
    using modelType=typename modelT::ModelType;
    using unitT=typename modelType::UnitType;
    static typename unitT::type sample;

    rdbModel->createObject=[model,allocatorFactory]
        (RocksdbHandler& handler, Topic topic, const dataunit::Unit* object, Transaction* tx)
    {
        const auto* obj=sample.castToUnit(object);
        return CreateObject(model->model,handler,topic,obj,allocatorFactory,tx);
    };

    rdbModel->readObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            Topic topic,
            const ObjectId& objectId,
            Transaction* tx,
            bool forUpdate
        )
    {
        auto r=ReadObject(model->model,handler,topic,objectId,hana::false_c,allocatorFactory,tx,forUpdate);
        if (r)
        {
            return Result<DbObject>{r.takeError()};
        }
        return Result<DbObject>{r.takeValue(),topic};
    };

    rdbModel->readObjectWithDate=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            Topic topic,
            const ObjectId& objectId,
            const HATN_COMMON_NAMESPACE::Date& date,
            Transaction* tx,
            bool forUpdate
        )
    {
        auto r=ReadObject(model->model,handler,topic,objectId,date,allocatorFactory,tx,forUpdate);
        if (r)
        {
            return Result<DbObject>{r.takeError()};
        }
        return Result<DbObject>{r.takeValue(),topic};
    };

    rdbModel->find=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const ModelIndexQuery& query,
            bool single
        )
    {
        return Find(model->model,handler,query,single,allocatorFactory);
    };

    rdbModel->findCb=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const ModelIndexQuery& query,
            const FindCb& cb,
            Transaction* tx,
            bool forUpdate
        )
    {
        return FindCbOp(model->model,handler,query,cb,allocatorFactory,tx,forUpdate);
    };

    rdbModel->deleteObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            Topic topic,
            const ObjectId& objectId,
            Transaction* tx
        )
    {
        return DeleteObject(model->model,handler,topic,objectId,hana::false_c,allocatorFactory,tx);
    };

    rdbModel->deleteObjectWithDate=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            Topic topic,
            const ObjectId& objectId,
            const HATN_COMMON_NAMESPACE::Date& date,
            Transaction* tx
        )
    {
        return DeleteObject(model->model,handler,topic,objectId,date,allocatorFactory,tx);
    };

    rdbModel->deleteMany=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const ModelIndexQuery& query,
            bool bulk,
            Transaction* tx
        )
    {
        return DeleteMany(model->model,handler,query,allocatorFactory,bulk,tx);
    };

    rdbModel->updateObjectWithDate=[model,allocatorFactory]
        (
             RocksdbHandler& handler,
             Topic topic,
             const ObjectId& objectId,
             const update::Request& request,
             const HATN_COMMON_NAMESPACE::Date& date,
             db::update::ModifyReturn modifyReturn,
             Transaction* tx
        )
    {
        return UpdateObject(model->model,handler,topic,objectId,request,date,modifyReturn,allocatorFactory,tx);
    };

    rdbModel->updateObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            Topic topic,
            const ObjectId& objectId,
            const update::Request& request,
            db::update::ModifyReturn modifyReturn,
            Transaction* tx
            )
    {
        return UpdateObject(model->model,handler,topic,objectId,request,hana::false_c,modifyReturn,allocatorFactory,tx);
    };

    rdbModel->updateMany=[model,allocatorFactory]
        (
           RocksdbHandler& handler,
           const ModelIndexQuery& query,
           const update::Request& request,
           db::update::ModifyReturn modifyReturnFirst,
           Transaction* tx
        )
    {
        auto r=UpdateMany(model->model,handler,query,request,modifyReturnFirst,allocatorFactory,tx);
        if (r)
        {
            return Result<size_t>{r.takeError()};
        }
        return Result<size_t>{r.value().first};
    };

    rdbModel->findUpdateCreate=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const ModelIndexQuery& query,
            const update::Request& request,
            const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
            db::update::ModifyReturn modifyReturn,
            Transaction* tx
        )
    {
        if (modifyReturn==db::update::ModifyReturn::None)
        {
            modifyReturn=db::update::ModifyReturn::Before;
        }

        // try update
        auto r=UpdateMany(model->model,handler,query,request,modifyReturn,allocatorFactory,tx,true);
        if (r)
        {
            return Result<DbObject>{r.takeError()};
        }
        if (r.value().second)
        {
            return Result<DbObject>{r.takeValue().second};
        }

        // done if object is not provided
        if (object.isNull())
        {
            return Result<DbObject>{DbObject{}};
        }

        // create if not found
        Assert(!query.query.topics().empty(),"Topic must be provided");
        Topic topic{query.query.topics().at(0)};
        const auto* obj=sample.castToUnit(object.get());
        auto&& ec=CreateObject(model->model,handler,topic,obj,allocatorFactory,tx);
        if (ec)
        {
            return Result<DbObject>{std::move(ec)};
        }
        if (modifyReturn==db::update::ModifyReturn::Before)
        {
            return Result<DbObject>{DbObject{}};
        }
        return Result<DbObject>{object,topic};
    };

    rdbModel->count=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const ModelIndexQuery& query
        )
    {
        return Count(model->model,handler,query,allocatorFactory);
    };

    using mType=std::decay_t<decltype(model->model)>;
    RocksdbModelT<mType>::init(model->model);

    m_models[model->info->modelId()]=std::move(rdbModel);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELS_IPP
