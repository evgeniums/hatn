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

#include <hatn/dataunit/compare.h>
#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/name.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const char* InvitationKey="invitation_key";

HDU_UNIT(public_key,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
    HDU_FIELD(content,TYPE_BYTES,5)
    HDU_FIELD(content_format,TYPE_STRING,6)
)

HDU_UNIT_WITH(private_key,(HDU_BASE(public_key)),
    HDU_FIELD(passphrase,TYPE_STRING,10)
)

HDU_UNIT_WITH(private_key_db,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(private_key)),
)

HDU_UNIT(encrypted,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(index,TYPE_UINT64,3)
    HDU_FIELD(nonce,TYPE_BYTES,4)
    HDU_FIELD(content,TYPE_BYTES,5)
)

HDU_UNIT(encryptable_string,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,with_string::TYPE,2)
)

HDU_UNIT(encryptable_object,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,TYPE_DATAUNIT,2)
)

template <typename T1, typename T2>
bool encryptableStringsEqual(const T1& l, const T2& r)
{
    if (!HATN_DATAUNIT_NAMESPACE::fieldEqual(encryptable_string::plain,l,r))
    {
        return false;
    }
    return HATN_DATAUNIT_NAMESPACE::subunitsEqual(encryptable_string::encrypted,l,r);
}

template <typename FieldT, typename T1, typename T2>
bool encryptableStringFieldsEqual(const FieldT& field, const T1& l, const T2& r)
{
    if (!l)
    {
        if (!r)
        {
            return false;
        }
        return true;
    }
    if (!r)
    {
        return false;
    }

    const auto* lf=&l->field(field).value();
    const auto* rf=&r->field(field).value();
    return encryptableStringsEqual(lf,rf);
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSENCRYPTED_H
