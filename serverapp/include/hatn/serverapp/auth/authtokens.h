/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/authtokens.h
  */

/****************************************************************************/

#ifndef HATNSERVERAUTHTOKENS_H
#define HATNSERVERAUTHTOKENS_H

#include <hatn/dataunit/syntax.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

HDU_UNIT(auth_token,
    HDU_FIELD(id,TYPE_OBJECT_ID,1,true)
    HDU_FIELD(session,TYPE_OBJECT_ID,2,true)
    HDU_FIELD(created_at,TYPE_DATETIME,3,true)
    HDU_FIELD(session_created_at,TYPE_DATETIME,4,true)
    HDU_FIELD(expire,TYPE_DATETIME,5,true)
    HDU_FIELD(login,TYPE_OBJECT_ID,6,true)
    HDU_FIELD(username,TYPE_STRING,7)
    HDU_FIELD(topic,TYPE_STRING,8)
    HDU_ENUM(TokenType,Session,Refresh)
    HDU_FIELD(token_type,HDU_TYPE_ENUM(TokenType),9,false,TokenType::Session)
)

HDU_UNIT(auth_challenge_token,
    HDU_FIELD(id,TYPE_OBJECT_ID,1,true)
    HDU_FIELD(token_created_at,TYPE_DATETIME,2,true)
    HDU_FIELD(expire,TYPE_DATETIME,3,true)
    HDU_FIELD(login,TYPE_STRING,4)
    HDU_FIELD(topic,TYPE_STRING,5)
    HDU_FIELD(challenge,TYPE_BYTES,6,true)
)

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERAUTHTOKENS_H
