/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/withloginprofile.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERWITHLOGINPROFILE_H
#define HATNCLIENTSERVERWITHLOGINPROFILE_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_login_profile,
    HDU_FIELD(login_profile,TYPE_OBJECT_ID,1201)
    HDU_FIELD(login_profile_topic,TYPE_OBJECT_ID,1202)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERWITHLOGINPROFILE_H
