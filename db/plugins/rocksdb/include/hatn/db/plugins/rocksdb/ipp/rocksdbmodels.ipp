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
#include <hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdeletemany.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbupdate.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbupdatemany.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

#include <hatn/db/plugins/rocksdb/rocksdbmodels.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ModelT>
void RocksdbModels::registerModel(std::shared_ptr<ModelWithInfo<ModelT>> model,
                                  AllocatorFactory* allocatorFactory)
{
    Assert(m_models.find(model->info->modelId())==m_models.end(),"Failed to register duplicate model");

    auto rdbModel=std::make_shared<RocksdbModel>(model->info);

    using modelT=typename std::decay_t<decltype(model)>::element_type;
    using modelType=typename modelT::ModelType;
    using unitT=typename modelType::UnitType;
    static typename unitT::type sample;

    rdbModel->createObject=[model,allocatorFactory]
        (RocksdbHandler& handler, const Topic& topic, const dataunit::Unit* object, Transaction* tx)
    {
        const auto* obj=sample.castToUnit(object);
        return CreateObject(model->model,handler,topic,obj,allocatorFactory,tx);
    };

    rdbModel->readObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const Topic& topic,
            const ObjectId& objectId,
            Transaction* tx,
            bool forUpdate
        )
    {
        auto r=ReadObject(model->model,handler,topic,objectId,hana::false_c,allocatorFactory,tx,forUpdate);
        if (r)
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
    };

    rdbModel->readObjectWithDate=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const Topic& topic,
            const ObjectId& objectId,
            const HATN_COMMON_NAMESPACE::Date& date,
            Transaction* tx,
            bool forUpdate
        )
    {
        auto r=ReadObject(model->model,handler,topic,objectId,date,allocatorFactory,tx,forUpdate);
        if (r)
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
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

    rdbModel->deleteObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const Topic& topic,
            const ObjectId& objectId,
            Transaction* tx
        )
    {
        return DeleteObject(model->model,handler,topic,objectId,hana::false_c,allocatorFactory,tx);
    };

    rdbModel->deleteObjectWithDate=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const Topic& topic,
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
            Transaction* tx
        )
    {
        return DeleteMany(model->model,handler,query,allocatorFactory,tx);
    };

    rdbModel->updateObjectWithDate=[model,allocatorFactory]
        (
             RocksdbHandler& handler,
             const Topic& topic,
             const ObjectId& objectId,
             const update::Request& request,
             const HATN_COMMON_NAMESPACE::Date& date,
             db::update::ModifyReturn modifyReturn,
             Transaction* tx
        )
    {
        auto r=UpdateObject(model->model,handler,topic,objectId,request,date,modifyReturn,allocatorFactory,tx);
        if (r)
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
    };

    rdbModel->updateObject=[model,allocatorFactory]
        (
            RocksdbHandler& handler,
            const Topic& topic,
            const ObjectId& objectId,
            const update::Request& request,
            db::update::ModifyReturn modifyReturn,
            Transaction* tx
            )
    {
        auto r=UpdateObject(model->model,handler,topic,objectId,request,hana::false_c,modifyReturn,allocatorFactory,tx);
        if (r)
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
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
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
    };

    rdbModel->readUpdateCreate=[model,allocatorFactory]
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
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
        }
        if (r.value())
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue()};
        }

        // create if not found
        Topic topic;
        if (!query.query.topics().empty())
        {
            topic=query.query.topics().at(0);
        }
        const auto* obj=sample.castToUnit(object.get());
        auto&& ec=CreateObject(model->model,handler,topic,obj,allocatorFactory,tx);
        if (ec)
        {
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{std::move(ec)};
        }
        return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{object};
    };

    using mType=std::decay_t<decltype(model->model)>;
    RocksdbModelT<mType>::init(model->model);

    m_models[model->info->modelId()]=std::move(rdbModel);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELS_IPP
