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

//! Notification settings of one level (account, character or chat).
//!
//! mode is a bit mask of NotificationMode: DisableSound shows notifications without sound,
//! DisableNotifications mutes them until cleared. disable_until is a timed mute. The two bits and
//! the timed mute are independent, see whitemclient/chat/notificationmute.h for how they combine.
//! revision is stamped by the server on every change and is how a device tells newer settings
//! from older ones; an empty revision means the settings were never changed.
HDU_UNIT(notifications,
    HDU_FIELD(mode,HDU_TYPE_ENUM(NotificationMode),1,false,NotificationMode::Normal)
    HDU_FIELD(disable_until,TYPE_DATETIME,2)
    HDU_FIELD(sound,TYPE_STRING,3)
    HDU_FIELD(revision,TYPE_STRING,4)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELNOTIFICATIONS_H
