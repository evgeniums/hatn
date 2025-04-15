/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminclient/admin.h
  */

/****************************************************************************/

#ifndef HATNADMINCLIENTADMIN_H
#define HATNADMINCLIENTADMIN_H

#include <hatn/db/object.h>

#include <hatn/adminclient/adminclient.h>

HATN_ADMIN_CLIENT_NAMESPACE_BEGIN

enum class AuthMode : uint8_t
{
    Password=0,
    Pubkey=1,
    Certificate=2
};

HDU_UNIT_WITH(admin,(HDU_BASE(db::object)),
    HDU_FIELD(blocked,TYPE_BOOL,1)
    HDU_FIELD(alias,TYPE_STRING,2)
    HDU_FIELD(enable_remote_login,TYPE_BOOL,3)
    HDU_FIELD(auth_mode,HDU_TYPE_ENUM(AuthMode),4,false,AuthMode::Password)
    HDU_FIELD(password,TYPE_STRING,5)
    HDU_FIELD(pubkey,TYPE_STRING,6)
    HDU_FIELD(certificate,TYPE_STRING,7)
    HDU_FIELD(cipher_suite,TYPE_STRING,8)
    HDU_FIELD(name,TYPE_STRING,9)
)

HDU_UNIT_WITH(admin_group,(HDU_BASE(db::object)),
    HDU_FIELD(admin,TYPE_OBJECT_ID,1)
    HDU_FIELD(group,TYPE_OBJECT_ID,2)
    HDU_FIELD(roles,TYPE_UINT32,3)
)

using Admin=admin::managed;
using AdminGroup=admin_group::managed;

HATN_ADMIN_CLIENT_NAMESPACE_END

#endif // HATNADMINCLIENTADMIN_H
