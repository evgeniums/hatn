/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/unreadcount.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELUNREADCOUNT_H
#define HATNCLIENTSERVERMODELUNREADCOUNT_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const int UnreadMessagesFieldId=210;
constexpr const int UnreadRervisionFieldId=211;

HDU_UNIT(unread_count,
    HDU_FIELD(unread_messages,TYPE_UINT32,UnreadMessagesFieldId)
    HDU_FIELD(unread_revision,TYPE_OBJECT_ID,UnreadRervisionFieldId)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELUNREADCOUNT_H
