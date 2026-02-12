/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/cachedbmodelsprovider.сpp
  *
  */

#include <hatn/clientserver/models/cachedbmodel.h>

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

void CacheDbModelsProvider::initCollections(const std::set<std::string>& collections)
{
    for (const auto& collection: collections)
    {
        m_models.emplace(collection,std::make_shared<DbModelType>(makeCacheModel(collection)));
    }
}

//--------------------------------------------------------------------------

CacheDbModelsProvider::DbModelType* CacheDbModelsProvider::model(const std::string& collection)
{
    auto it=m_models.find(collection);
    if (it==m_models.end())
    {
        return nullptr;
    }
    return it->second.get();
}

//--------------------------------------------------------------------------

std::vector<std::shared_ptr<HATN_DB_NAMESPACE::ModelInfo>> CacheDbModelsProvider::models() const
{
    std::vector<std::shared_ptr<HATN_DB_NAMESPACE::ModelInfo>> res;
    for (const auto& it: m_models)
    {
        auto model=*it.second;
        res.push_back(model->info);
    }
    return res;
}

//--------------------------------------------------------------------------

void CacheDbModelsProvider::registerRocksdbModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    for (const auto& it: m_models)
    {
        auto model=*it.second;
        HATN_ROCKSDB_NAMESPACE::RocksdbModels::instance().registerModel(model);
    }
#endif
}

//--------------------------------------------------------------------------

void CacheDbModelsProvider::unregisterRocksdbModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    for (const auto& it: m_models)
    {
        auto model=*it.second;
        HATN_ROCKSDB_NAMESPACE::RocksdbModels::instance().unregisterModel(model);
    }
#endif
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
