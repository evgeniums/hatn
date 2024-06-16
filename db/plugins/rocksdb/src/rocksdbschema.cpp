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

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>
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
{
}

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
    Assert(m_models.find(model->info())==m_models.end(),"duplicate model");
    m_models.emplace(model->info(),std::move(model));
}

//---------------------------------------------------------------

std::shared_ptr<RocksdbModel> RocksdbSchema::findModel(const db::ModelInfo& info) const
{
    auto it=m_models.find(info);
    if (it==m_models.end())
    {
        return std::shared_ptr<RocksdbModel>{};
    }

    return it->second;
}

/********************** RocksdbModel **************************/

//---------------------------------------------------------------

RocksdbModel::RocksdbModel(db::ModelInfo& info)
    :m_modelInfo(info)
{
    m_modelInfo.setNativeModel(this);
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
