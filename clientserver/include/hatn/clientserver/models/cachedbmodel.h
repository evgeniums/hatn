/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/cachedbmodel.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELCACHEDB_H
#define HATNCLIENTSERVERMODELCACHEDB_H

#include <hatn/db/object.h>
#include <hatn/db/modelsprovider.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/genericdbmodels.h>
#include <hatn/clientserver/models/cache.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

inline auto makeCacheModel(std::string collection)
{
    auto cfg=HATN_DB_NAMESPACE::DefaultModelConfig;
    cfg.setCollection(std::move(collection));
    auto m=HATN_DB_NAMESPACE::makeModel<cache_object::TYPE>(cfg,
                                                              uidIdsIdx(),
                                                              expireIdx(),
                                                              revisionIdx());
    return m;
}

class HATN_CLIENT_SERVER_EXPORT CacheDbModelsProvider : public HATN_DB_NAMESPACE::ModelsProvider
{
    public:

        using DbModelType=decltype(makeCacheModel(std::string{}));

        using HATN_DB_NAMESPACE::ModelsProvider::ModelsProvider;

        void initCollections(const std::set<std::string>& collections);

        void registerRocksdbModels() override;
        void unregisterRocksdbModels() override;

        std::vector<std::shared_ptr<HATN_DB_NAMESPACE::ModelInfo>> models() const override;

        DbModelType* model(const std::string& collection);

    private:

        common::FlatMap<std::string,std::shared_ptr<DbModelType>> m_models;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELCACHEDB_H
