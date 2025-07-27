/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/usercharacter.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERUSERCHARACTER_H
#define HATNCLIENTSERVERUSERCHARACTER_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/withuser.h>
#include <hatn/clientserver/models/withusercharacter.h>
#include <hatn/clientserver/models/withloginprofile.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT_WITH(user_character,(HDU_BASE(db::object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,2)
)

HDU_UNIT_WITH(user_character_server,(HDU_BASE(with_user),HDU_BASE(user_character)),
    HDU_FIELD(blocked,TYPE_BOOL,30)
    HDU_FIELD(server_description,TYPE_STRING,31)
)

HDU_UNIT_WITH(user_character_share,(HDU_BASE(db::object),HDU_BASE(with_user),HDU_BASE(with_user_character)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(access,TYPE_UINT32,2)
    HDU_FIELD(description,TYPE_STRING,3)
)

HDU_UNIT_WITH(user_character_login,(HDU_BASE(db::object),HDU_BASE(with_user_character),HDU_BASE(with_login_profile)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,3)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERUSERCHARACTER_H
