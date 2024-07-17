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

#include <hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbreadobject.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbfind.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbdelete.ipp>

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
        using unitT=typename modelT::ModelType::UnitType;
        static typename unitT::type sample;

        rdbModel->createObject=[model,allocatorFactory]
            (RocksdbHandler& handler, const Namespace& ns, const dataunit::Unit* object)
        {
            const auto* obj=sample.castToUnit(object);
            return CreateObject<BufT>(model->model,handler,ns,obj,allocatorFactory);
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
                const ObjectId& objectId
            )
        {
            return DeleteObject<BufT>(model->model,handler,ns,objectId,hana::false_c,allocatorFactory);
        };

        rdbModel->deleteObjectWithDate=[model,allocatorFactory]
            (
                RocksdbHandler& handler,
                const Namespace& ns,
                const ObjectId& objectId,
                const HATN_COMMON_NAMESPACE::Date& date
            )
        {
            return DeleteObject<BufT>(model->model,handler,ns,objectId,date,allocatorFactory);
        };

        rdbSchema->addModel(std::move(rdbModel));
    };

    auto& models=schema->models();
    boost::hana::for_each(models,addModel);

    m_schemas[schema->name()]=std::move(rdbSchema);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_IPP
