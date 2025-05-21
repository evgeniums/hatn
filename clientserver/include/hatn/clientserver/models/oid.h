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

HDU_UNIT(oid,
    HDU_FIELD(_id,TYPE_OBJECT_ID,db::ObjectIdFieldId)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSOID_H
