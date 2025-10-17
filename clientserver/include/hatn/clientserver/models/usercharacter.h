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
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/models/revision.h>
#include <hatn/clientserver/models/name.h>
#include <hatn/clientserver/models/username.h>
#include <hatn/clientserver/models/addressitem.h>
#include <hatn/clientserver/models/employment.h>
#include <hatn/clientserver/models/notifications.h>
#include <hatn/clientserver/models/unreadcount.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT_WITH(user_character_public,(HDU_BASE(with_name),HDU_BASE(with_username),HDU_BASE(with_revision)),
    HDU_FIELD(avatar,topic_object::TYPE,1)
    HDU_FIELD(notes,TYPE_STRING,4)
    HDU_REPEATED_FIELD(addresses,address_item::TYPE,5)
    HDU_FIELD(employment,employment::TYPE,6)
)

HDU_UNIT_WITH(user_character_private,(HDU_BASE(with_revision)),
    HDU_FIELD(encrypted_notes,encrypted::TYPE,1)
    HDU_FIELD(plain_notes,TYPE_STRING,2)
    HDU_FIELD(shared_from,with_user_character::TYPE,3)
    HDU_FIELD(notifications,notifications::TYPE,4)
)

HDU_UNIT_WITH(user_character_full,(HDU_BASE(db::object)),
     HDU_FIELD(public_data,user_character_public::TYPE,1)
     HDU_FIELD(private_data,user_character_private::TYPE,2)
)

HDU_UNIT_WITH(user_character_server_db,(HDU_BASE(with_user),HDU_BASE(user_character_full)),
    HDU_FIELD(server_blocked,TYPE_BOOL,30)
    HDU_FIELD(server_notes,TYPE_STRING,31)
    HDU_FIELD(server_permissions,TYPE_UINT64,32)
)

HDU_UNIT_WITH(user_character_public_sync,(HDU_BASE(oid)),
    HDU_FIELD(public_data,user_character_public::TYPE,1)
)

HDU_UNIT_WITH(user_character_private_sync,(HDU_BASE(user_character_full),HDU_BASE(unread_count))
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
