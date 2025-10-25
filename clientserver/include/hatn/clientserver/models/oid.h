/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/oid.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSOID_H
#define HATNCLIENTSERVERMODELSOID_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const int ServerOidFieldId=110;

HDU_UNIT(oid,
    HDU_FIELD(_id,TYPE_OBJECT_ID,db::ObjectIdFieldId)
)

HDU_UNIT(at_server,
    HDU_FIELD(server_oid,TYPE_OBJECT_ID,ServerOidFieldId)
)

HDU_UNIT(oid_key,
    HDU_FIELD(oid,TYPE_OBJECT_ID,99)
)

HDU_UNIT(with_parent,
    HDU_FIELD(parent_oid,TYPE_OBJECT_ID,99)
    HDU_FIELD(parent_type,TYPE_STRING,98)
)

HDU_UNIT_WITH(topic_object,(HDU_BASE(with_parent)),
    HDU_FIELD(oid,TYPE_OBJECT_ID,1)
    HDU_FIELD(topic,TYPE_STRING,2)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSOID_H
