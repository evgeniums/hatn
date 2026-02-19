/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/genericdbmodels.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERGENERICDBMODELS_H
#define HATNCLIENTSERVERGENERICDBMODELS_H

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/models/expire.h>
#include <hatn/clientserver/models/revision.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HATN_DB_TTL_INDEX(expireIdx,1,with_expire::expire_at)
HATN_DB_UNIQUE_INDEX(uidIdx,with_uid_idx::ids,
                     HATN_DB_NAMESPACE::nested(with_uid::uid,uid::version),
                     HATN_DB_NAMESPACE::nested(with_uid::uid,uid::index))
HATN_DB_INDEX(parentUidIdx,with_parent_idx::parent_ids)
HATN_DB_INDEX(revisionIdx,with_revision::revision)
HATN_DB_INDEX(uidDateIdx,HATN_DB_NAMESPACE::nested(with_uid::uid,uid::date))
HATN_DB_PARTITION_INDEX(uidDateIdxP,HATN_DB_NAMESPACE::nested(with_uid::uid,uid::date))
HATN_DB_INDEX(parentIdx,with_parent::parent_oid,with_parent::parent_topic)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERGENERICDBMODELS_H
