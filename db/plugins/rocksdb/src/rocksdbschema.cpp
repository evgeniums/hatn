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

/********************** RocksdbSchema **************************/

//---------------------------------------------------------------

void RocksdbSchema::addModel(std::shared_ptr<RocksdbModel> model)
{
    Assert(m_models.find(model->cuid())!=m_models.end(),"duplicate model");
    m_models.emplace(model->cuid(),std::move(model));
}

//---------------------------------------------------------------

Result<std::shared_ptr<RocksdbModel>> RocksdbSchema::model(CUID_TYPE cuid) const noexcept
{
    auto it=m_models.find(cuid);
    if (it==m_models.end())
    {
        return dbError(DbError::MODEL_NOT_FOUND);
    }

    return it->second;
}

/********************** RocksdbModel **************************/

//---------------------------------------------------------------

RocksdbModel::RocksdbModel()
{}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
