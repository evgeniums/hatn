/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbmodels.cpp
  *
  *   RocksDB database models singleton.
  *
  */

/****************************************************************************/

#include <hatn/common/utils.h>

#include <hatn/db/plugins/rocksdb/rocksdbmodels.h>

HATN_DB_USING

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbModel **************************/

//---------------------------------------------------------------

RocksdbModel::RocksdbModel(std::shared_ptr<ModelInfo> info, const AllocatorFactory* factory)
    : common::pmr::WithFactory(factory),
      m_modelInfo(std::move(info))
{
    m_modelInfo->setNativeModel(this);
}

/********************** RocksdbModels **************************/

//---------------------------------------------------------------

HATN_SINGLETON_INIT(RocksdbModels)

//---------------------------------------------------------------

RocksdbModels& RocksdbModels::instance()
{
    static RocksdbModels inst;
    return inst;
}

//---------------------------------------------------------------

void RocksdbModels::free() noexcept
{
    instance().m_models.clear();
}

//---------------------------------------------------------------

RocksdbModels::RocksdbModels()
{
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
