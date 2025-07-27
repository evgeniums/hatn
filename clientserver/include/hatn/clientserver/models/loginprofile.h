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

HDU_UNIT(login_profile,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(auth_scheme,api::auth_protocol::TYPE,2)
    HDU_FIELD(expire_at,TYPE_DATETIME,3)
    HDU_FIELD(description,TYPE_STRING,4)
    HDU_FIELD(parameter1,TYPE_STRING,5)
    HDU_FIELD(parameter2,TYPE_STRING,6)
    HDU_FIELD(secret1,TYPE_STRING,7)
    HDU_FIELD(secret2,TYPE_STRING,8)
)

HDU_UNIT_WITH(user_login,(HDU_BASE(db::object),HDU_BASE(with_user),HDU_BASE(login_profile)),
    HDU_FIELD(blocked,TYPE_BOOL,30)
    HDU_FIELD(server_description,TYPE_STRING,31)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERLOGINPROFILE_H
