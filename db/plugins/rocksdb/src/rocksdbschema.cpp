/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbschema.cpp
  *
  *   RocksDB database schema handling source.
  *
  */

/****************************************************************************/

#include <hatn/common/utils.h>

#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbmodelt.h>
#include <hatn/db/plugins/rocksdb/rocksdbmodels.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

HATN_DB_USING

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbSchemas **************************/

//---------------------------------------------------------------

HATN_SINGLETON_INIT(RocksdbSchemas)

//---------------------------------------------------------------

RocksdbSchemas& RocksdbSchemas::instance()
{
    static RocksdbSchemas inst;
    return inst;
}

//---------------------------------------------------------------

void RocksdbSchemas::free() noexcept
{
    instance().m_schemas.clear();
}

//---------------------------------------------------------------

RocksdbSchemas::RocksdbSchemas()
{}

//---------------------------------------------------------------

std::shared_ptr<RocksdbSchema> RocksdbSchemas::schema(const lib::string_view &name) const
{
    auto it=m_schemas.find(name);
    if (it==m_schemas.end())
    {
        return std::shared_ptr<RocksdbSchema>{};
    }

    return it->second;
}

/********************** RocksdbSchema **************************/

//---------------------------------------------------------------

void RocksdbSchema::addModel(std::shared_ptr<RocksdbModel> model)
{
    auto it=m_models.find(model->info()->modelId());
    Assert(it==m_models.end(),"Model with such ID already registered in RocksDB schema");
    m_models[model->info()->modelId()]=std::move(model);
}

//---------------------------------------------------------------

void RocksdbSchemas::registerSchema(std::shared_ptr<Schema> schema)
{
    Assert(m_schemas.find(schema->name())==m_schemas.end(),"Failed to register duplicate database schema");

    auto rdbSchema=std::make_shared<RocksdbSchema>(schema);
    for (auto&& it:schema->models())
    {
        auto rdbModel=RocksdbModels::instance().model(it.second);
        Assert(static_cast<bool>(rdbModel),"RocksDB model is not registered, register it first with RocksdbModels::instance().registerModel()");

        rdbSchema->addModel(rdbModel);
    }
    m_schemas[schema->name()]=std::move(rdbSchema);
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
