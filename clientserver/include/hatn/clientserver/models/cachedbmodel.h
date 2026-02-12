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

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/cache.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HATN_DB_TTL_INDEX(cacheTtlIdx,1,with_expire::expire_at)
HATN_DB_UNIQUE_INDEX(cacheLocalIdx,cache_object::local_oid)
HATN_DB_UNIQUE_INDEX(cacheServerIdx,cache_object::server_hash)
HATN_DB_UNIQUE_INDEX(cacheGuidIdx,cache_object::guid_hash)
HATN_DB_UNIQUE_INDEX(cacheRevisionIdx,with_revision::revision)
HATN_DB_MODEL(cacheModel,cache_object,cacheTtlIdx(),cacheLocalIdx(),cacheServerIdx(),cacheGuidIdx(),cacheRevisionIdx())

inline auto makeCacheModel(std::string collection)
{
    auto cfg=HATN_DB_NAMESPACE::DefaultModelConfig;
    cfg.setCollection(std::move(collection));
    auto m=HATN_DB_NAMESPACE::makeModel<cache_object::TYPE>(cfg,
                                                              cacheTtlIdx(),
                                                              cacheLocalIdx(),
                                                              cacheServerIdx(),
                                                              cacheGuidIdx(),
                                                              cacheRevisionIdx());
    return m;
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELCACHEDB_H
