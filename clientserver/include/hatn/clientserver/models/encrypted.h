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

HDU_UNIT(public_key,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(cipher_id,TYPE_STRING,3)
    HDU_FIELD(content,TYPE_BYTES,5)
)

HDU_UNIT(encrypted,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(index,TYPE_UINT64,3)
    HDU_FIELD(nonce,TYPE_UINT64,4)
    HDU_FIELD(content,TYPE_BYTES,5)
)

HDU_UNIT(encryptable_string,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,TYPE_STRING,2)
)

HDU_UNIT(encryptable_object,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,TYPE_DATAUNIT,2)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSENCRYPTED_H
