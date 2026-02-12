/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/expire.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELEXPIRE_H
#define HATNCLIENTSERVERMODELEXPIRE_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_expire,
    HDU_FIELD(expire_at,TYPE_DATETIME,77)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELEXPIRE_H
