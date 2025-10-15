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
#include <hatn/clientserver/models/encrypted.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const char* CHARACTERNAME_SCHEMA_HOST="host";

HDU_UNIT(name,
    HDU_FIELD(first,TYPE_STRING,1)
    HDU_FIELD(middle,TYPE_STRING,2)
    HDU_FIELD(last,TYPE_STRING,3)
)

HDU_UNIT(address_item,
    HDU_FIELD(id,db::TYPE_OBJECT_ID,1)
    HDU_FIELD(value,TYPE_STRING,2)
    HDU_FIELD(address_type,TYPE_STRING,3)
    HDU_FIELD(title,TYPE_STRING,4)
)

HDU_UNIT(user_character,
    HDU_FIELD(name,name::TYPE,1)
    HDU_FIELD(charactername,TYPE_STRING,2)
    HDU_FIELD(charactername_schema,TYPE_STRING,3,false,CHARACTERNAME_SCHEMA_HOST)
    HDU_FIELD(notes,TYPE_STRING,4)
    HDU_REPEATED_FIELD(addresses,address_item::TYPE,5)
    HDU_FIELD(organization,TYPE_STRING,6)
    HDU_FIELD(department,TYPE_STRING,7)
    HDU_FIELD(job_position,TYPE_STRING,8)
)

HDU_UNIT_WITH(user_character_global,(HDU_BASE(db::object),HDU_BASE(user_character))
)

HDU_UNIT_WITH(user_character_server_db,(HDU_BASE(user_character_global)),
    HDU_FIELD(private_notes,encrypted::TYPE,20)
    HDU_FIELD(server_blocked,TYPE_BOOL,30)
    HDU_FIELD(server_notes,TYPE_STRING,31)
    HDU_FIELD(server_permissions,TYPE_UINT64,32)
)

HDU_UNIT(user_character_private,
    HDU_FIELD(global,user_character_global::TYPE,1)
    HDU_FIELD(private_notes,encrypted::TYPE,2)
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
