/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/loginprofile.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERLOGINPROFILE_H
#define HATNCLIENTSERVERLOGINPROFILE_H

#include <hatn/db/object.h>

#include <hatn/api/authunit.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/withuser.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT_WITH(login_profile,(HDU_BASE(db::object),HDU_BASE(with_user)),
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(blocked,TYPE_BOOL,3)
    HDU_FIELD(auth_scheme,api::auth_protocol::TYPE,4)
    HDU_FIELD(expire_at,TYPE_DATETIME,5)
    HDU_FIELD(description,TYPE_STRING,6)
    HDU_FIELD(parameter1,TYPE_STRING,7)
    HDU_FIELD(parameter2,TYPE_STRING,8)
    HDU_FIELD(secret1,TYPE_STRING,9)
    HDU_FIELD(secret2,TYPE_STRING,10)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERLOGINPROFILE_H
