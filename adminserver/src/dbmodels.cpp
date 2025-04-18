/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file adminserver/dbmodels.сpp
  *
  */

#include <hatn/db/config.h>

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>

#endif

#include <hatn/adminserver/admindb.h>
#include <hatn/adminserver/dbmodels.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace {

const auto& dbModels()
{
    static auto models=hana::make_tuple(
        adminModel,
        adminGroupModel
    );

    return models;
}

} // anonymous namespace

//--------------------------------------------------------------------------

std::vector<std::shared_ptr<db::ModelInfo>> DbModels::models() const
{
    return modelInfoList(dbModels());
}

//--------------------------------------------------------------------------

void DbModels::registerRocksdbModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    hana::for_each(
        dbModels(),
        [](const auto& model)
        {
            HATN_ROCKSDB_NAMESPACE::RocksdbModels::instance().registerModel(model());
        }
    );
#endif
}

//--------------------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END
