/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbschema.ipp
  *
  *   Contains definition of RocksDB schema.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSCHEMA_IPP
#define HATNROCKSDBSCHEMA_IPP

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

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename DbSchemaSharedPtrT,
         typename BufT
         >
void RocksdbSchemas::registerSchema(DbSchemaSharedPtrT schema, AllocatorFactory* allocatorFactory)
{
    Assert(m_schemas.find(schema->name())==m_schemas.end(),"Trying to register duplicate database schema");

    auto rdbSchema=std::make_shared<RocksdbSchema>(schema);

    auto addModel=[&rdbSchema,allocatorFactory](auto model)
    {
        auto rdbModel=std::make_shared<RocksdbModel>(model->info);

        using modelT=typename std::decay_t<decltype(model)>::element_type;
        using modelType=typename modelT::ModelType;
        using unitT=typename modelType::UnitType;
        static typename unitT::type sample;

        rdbModel->createObject=[model,allocatorFactory]
            (RocksdbHandler& handler, const Namespace& ns, const dataunit::Unit* object, Transaction* tx)
        {
            const auto* obj=sample.castToUnit(object);
            return CreateObject<BufT>(model->model,handler,ns,obj,allocatorFactory,tx);
        };

        rdbModel->readObject=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId
            )
        {
            auto r=ReadObject<BufT>(model->model,handler,ns,objectId,hana::false_c,allocatorFactory);
            if (r)
            {
                return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
            }
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue().template staticCast<dataunit::Unit>()};
        };

        rdbModel->readObjectWithDate=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId,
                const HATN_COMMON_NAMESPACE::Date& date
            )
        {
            auto r=ReadObject<BufT>(model->model,handler,ns,objectId,date,allocatorFactory);
            if (r)
            {
                return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
            }
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue().template staticCast<dataunit::Unit>()};
        };

        rdbModel->find=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                IndexQuery& query
            )
        {
            return Find<BufT>(model->model,handler,query,allocatorFactory);
        };

        rdbModel->deleteObject=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId,
                Transaction* tx
            )
        {
            return DeleteObject<BufT>(model->model,handler,ns,objectId,hana::false_c,allocatorFactory,tx);
        };

        rdbModel->deleteObjectWithDate=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId,
                const HATN_COMMON_NAMESPACE::Date& date,
                Transaction* tx
            )
        {
            return DeleteObject<BufT>(model->model,handler,ns,objectId,date,allocatorFactory,tx);
        };

        rdbModel->deleteMany=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                IndexQuery& query,
                Transaction* tx
            )
        {
            return DeleteMany<BufT>(model->model,handler,query,allocatorFactory,tx);
        };

        rdbModel->updateObjectWithDate=[model,allocatorFactory]
            (
                 RocksdbHandler& handler,
                 const Namespace& ns,
                 const ObjectId& objectId,
                 const update::Request& request,
                 const HATN_COMMON_NAMESPACE::Date& date,
                 db::update::ModifyReturn modifyReturn,
                 Transaction* tx
            )
        {
            auto r=UpdateObject<BufT>(model->model,handler,ns,objectId,request,date,modifyReturn,allocatorFactory,tx);
            if (r)
            {
                return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
            }
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue().template staticCast<dataunit::Unit>()};
        };

        rdbModel->updateObject=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId,
                const update::Request& request,
                db::update::ModifyReturn modifyReturn,
                Transaction* tx
                )
        {
            auto r=UpdateObject<BufT>(model->model,handler,ns,objectId,request,hana::false_c,modifyReturn,allocatorFactory,tx);
            if (r)
            {
                return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
            }
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue().template staticCast<dataunit::Unit>()};
        };

        rdbModel->updateMany=[model,allocatorFactory]
            (
               RocksdbHandler& handler,
               IndexQuery& query,
               const update::Request& request,
               db::update::ModifyReturn modifyReturnFirst,
               Transaction* tx
            )
        {
            auto r=UpdateMany<BufT>(model->model,handler,query,request,modifyReturnFirst,allocatorFactory,tx);
            if (r)
            {
                return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeError()};
            }
            return Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>>{r.takeValue().template staticCast<dataunit::Unit>()};
        };

        using mType=std::decay_t<decltype(model->model)>;
        RocksdbModelT<mType>::init(model->model);

        rdbSchema->addModel(std::move(rdbModel));
    };

    auto& models=schema->models();
    boost::hana::for_each(models,addModel);

    m_schemas[schema->name()]=std::move(rdbSchema);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_IPP
