/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/withusercharacter.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERWITHUSERCHARACTER_H
#define HATNCLIENTSERVERWITHUSERCHARACTER_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_user_character,
    HDU_FIELD(user_character,TYPE_OBJECT_ID,1101)
    HDU_FIELD(user_character_topic,TYPE_OBJECT_ID,1102)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERWITHUSERCHARACTER_H
