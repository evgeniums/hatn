/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/cache.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELCACHE_H
#define HATNCLIENTSERVERMODELCACHE_H

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/expire.h>
#include <hatn/clientserver/models/revision.h>
#include <hatn/clientserver/models/oid.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT_WITH(cache_object,(HDU_BASE(HATN_DB_NAMESPACE::object),
                             HDU_BASE(with_expire),
                             HDU_BASE(with_revision),
                             HDU_BASE(with_uid),
                             HDU_BASE(with_uid_idx)
                             ),
    HDU_FIELD(data_type,TYPE_STRING,1)
    HDU_FIELD(data,TYPE_DATAUNIT,2)
    HDU_FIELD(deleted,TYPE_BOOL,3)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELCACHE_H
