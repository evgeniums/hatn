/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/withuser.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERWITHUSER_H
#define HATNCLIENTSERVERWITHUSER_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_user,
    HDU_FIELD(user,TYPE_OBJECT_ID,1001)
    HDU_FIELD(user_topic,TYPE_OBJECT_ID,1002)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERWITHUSER_H
