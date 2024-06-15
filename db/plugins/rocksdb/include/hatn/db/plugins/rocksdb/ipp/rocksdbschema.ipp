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

template <typename SchemaT>
static void RocksdbSchemas::registerSchema(const SchemaT& schema)
{
    Assert(m_schemas.find(schema.name)!=m_schemas.end(),"duplicate schema");

    RocksdbSchema rdbSchema;

    auto addModel=[&rdbSchema](auto&& model)
    {
        auto rdbModel=std::make_shared<RocksdbModel>(model.info);
        auto m=model.model;

        using modelT=std::decay_t<decltype(m)>;
        using unitT=typename m::UnitType;
        static unitT sample;

        rdbModel->createObject=[m](RocksdbHandler& handler, const db::Namespace& ns, dataunit::Unit* object)
        {
            auto* obj=sample.castToUnit(object);
            return CreateObject(m,handler,ns,obj);
        };

        rdbSchema.addModel(rdbModel);
    };

    boost::hana::for_each(schema.models,addModel);

    m_schemas[schema.name]=std::move(rdbSchema);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_IPP
