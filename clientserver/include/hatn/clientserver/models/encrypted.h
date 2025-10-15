/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/encrypted.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSENCRYPTED_H
#define HATNCLIENTSERVERMODELSENCRYPTED_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(encrypted,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(index,TYPE_UINT64,3)
    HDU_FIELD(nonce,TYPE_UINT64,4)
    HDU_FIELD(content,TYPE_BYTES,5)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSENCRYPTED_H
