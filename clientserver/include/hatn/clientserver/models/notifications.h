/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/notifications.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELNOTIFICATIONS_H
#define HATNCLIENTSERVERMODELNOTIFICATIONS_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

enum class NotificationMode
{
    Normal=0,
    DisableSound=0x1,
    DisableNotifications=0x2
};

HDU_UNIT(notifications,
    HDU_FIELD(mode,HDU_TYPE_ENUM(NotificationMode),1,false,NotificationMode::Normal)
    HDU_FIELD(mute_until,TYPE_DATETIME,2)
    HDU_FIELD(sound,TYPE_STRING,3)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELNOTIFICATIONS_H
