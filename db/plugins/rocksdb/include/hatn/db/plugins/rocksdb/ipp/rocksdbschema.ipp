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

#include <hatn/db/plugins/rocksdb/detail/rocksdbcreateobject.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename DbSchemaSharedPtrT,
         typename BufT,
         typename AllocatorT
         >
void RocksdbSchemas::registerSchema(DbSchemaSharedPtrT schema, const AllocatorT& alloc)
{
    Assert(m_schemas.find(schema->name())==m_schemas.end(),"Trying to register duplicate database schema");

    auto rdbSchema=std::make_shared<RocksdbSchema>(schema);

    auto addModel=[&rdbSchema,&alloc](auto model)
    {
        auto rdbModel=std::make_shared<RocksdbModel>(model->info);

        using modelT=typename std::decay_t<decltype(model)>::element_type;
        using unitT=typename modelT::ModelType::UnitType;
        static typename unitT::type sample;

        rdbModel->createObject=[model,&alloc](RocksdbHandler& handler, const db::Namespace& ns, dataunit::Unit* object)
        {
            auto* obj=sample.castToUnit(object);
            return CreateObject<BufT>(model->model,handler,ns,obj,alloc);
        };

        rdbSchema->addModel(std::move(rdbModel));
    };

    auto& models=schema->models();
    boost::hana::for_each(models,addModel);

    m_schemas[schema->name()]=std::move(rdbSchema);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_IPP
