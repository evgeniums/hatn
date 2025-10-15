/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/addressitem.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERADDRESSITEM_H
#define HATNCLIENTSERVERADDRESSITEM_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(address_item,
    HDU_FIELD(id,db::TYPE_OBJECT_ID,1)
    HDU_FIELD(value,TYPE_STRING,2)
    HDU_FIELD(address_type,TYPE_STRING,3)
    HDU_FIELD(title,TYPE_STRING,4)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERADDRESSITEM_H
