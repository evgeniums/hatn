/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/user.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSUSER_H
#define HATNCLIENTSERVERMODELSUSER_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT_WITH(user_profile,(HDU_BASE(db::object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(default_login_profile,TYPE_OBJECT_ID,2)
    HDU_FIELD(default_character,TYPE_OBJECT_ID,3)
    HDU_FIELD(email,TYPE_STRING,4)
    HDU_FIELD(phone,TYPE_STRING,5)
)

HDU_UNIT_WITH(user,(HDU_BASE(user_profile)),
    HDU_FIELD(reference_type,TYPE_STRING,200)
    HDU_FIELD(reference,TYPE_STRING,201)
    HDU_FIELD(blocked,TYPE_BOOL,202)
    HDU_FIELD(comments,TYPE_STRING,203)
)

using User=user::managed;

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSUSER_H
