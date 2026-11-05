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

constexpr const char* InvitationKey="invitation";
constexpr const char* ContactChatKey="contact_chat";

HDU_UNIT(key_info,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
)

HDU_UNIT(public_key,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
    HDU_FIELD(content,TYPE_BYTES,5)
    HDU_FIELD(content_format,TYPE_STRING,6)
)

HDU_UNIT_WITH(encryption_key,(HDU_BASE(public_key)),
    HDU_FIELD(secret,TYPE_BYTES,7)
)

HDU_UNIT_WITH(private_key_db,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(encryption_key)),
)

HDU_UNIT(encrypted,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(index,TYPE_UINT64,3)
    HDU_FIELD(nonce,TYPE_BYTES,4)
    HDU_FIELD(content,TYPE_BYTES,5)
    //! Id of the cipher suite the ciphertext and the keys that wrap its key were made with. Opaque to the server.
    HDU_FIELD(cipher_suite,TYPE_STRING,6)
    //! Key commitment: a 32 byte value derived from the content key and key_id, which a receiver
    //! checks before decrypting, so that one ciphertext cannot open to different plaintexts under
    //! different keys. Opaque to the server.
    HDU_FIELD(commitment,TYPE_BYTES,7)
)

/**
 * A content or notification key wrapped for one device key: one entry of the lists that accompany an
 * encrypted message. The server reads only the two key ids, to route and check the entries, and never
 * opens the wrapped key. Tags and wire types must match the server's common.v1.KeyWrap.
 */
HDU_UNIT(key_wrap,
    HDU_FIELD(recipient_key_id,TYPE_OBJECT_ID,1) //!< Device key the entry is for.
    HDU_FIELD(sender_key_id,TYPE_OBJECT_ID,2)    //!< Device key of the sender used in the key agreement.
    HDU_FIELD(epoch,TYPE_UINT32,3)               //!< Content or notification epoch of the wrapping key.
    HDU_FIELD(wrapped,TYPE_BYTES,4)              //!< The wrapped key, authenticated with the entry's data.
    HDU_FIELD(seq,TYPE_UINT32,5)                 //!< Position in a sender's chain; on-device chats only, epoch is unset then.
)

/**
 * A public key of the key registry, as the registry sends and receives it. Tags and wire types must
 * match the server's common.v1.PubKey.
 *
 * Not the same unit as public_key above: that one is stored in databases and in invitations with its
 * own tags (content 5, content_format 6) and cannot be renumbered.
 */
HDU_UNIT(pub_key,
    HDU_FIELD(key_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(key_type,TYPE_STRING,2)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
    HDU_FIELD(content,TYPE_BYTES,4)            //!< DER SubjectPublicKeyInfo.
    HDU_FIELD(content_format,TYPE_STRING,5)
    HDU_FIELD(created_at,TYPE_INT64,6)         //!< Unix seconds; set by the server in responses.
    HDU_FIELD(signer_key_id,TYPE_OBJECT_ID,7)  //!< Signing key that signed this key; empty for a signing key itself.
    HDU_FIELD(signature,TYPE_BYTES,8)          //!< DER ECDSA signature of the key's signed data.
    HDU_FIELD(generation,TYPE_UINT64,9)        //!< Chat keys of on-device chats only.
    HDU_FIELD(device_key_id,TYPE_OBJECT_ID,10) //!< Chat keys of on-device chats only: the device key it was made with.
)

HDU_UNIT(encryptable_string,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,with_string::TYPE,2)
)

HDU_UNIT(encryptable_object,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,TYPE_DATAUNIT,2)
)

HDU_UNIT(encryptable_string_asym,
    HDU_FIELD(encrypted,encrypted::TYPE,1)
    HDU_FIELD(plain,with_string::TYPE,2)
    HDU_FIELD(dh_pub_key,TYPE_STRING,3)
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
