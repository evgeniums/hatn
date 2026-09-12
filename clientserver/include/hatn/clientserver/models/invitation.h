/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/invitation.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELINVITATION_H
#define HATNCLIENTSERVERMODELINVITATION_H

#include <hatn/db/object.h>
#include <hatn/db/expire.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/encrypted.h>
#include <hatn/clientserver/models/serverhost.h>
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/models/usercharacter.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Format magic of every serialized shared_invitation, of every kind and version -- see
//! SharedInvitationKind below for why this is deliberately NOT namespaced per kind.
constexpr const char* InvitationPrefix="HINV";

/**
 * @brief Subtype of a shared invitation blob (shared_invitation::kind).
 *
 * Character==0 is the only value any producer emitted before this field existed, so an ABSENT
 * field must read as Character -- which the HDU default on shared_invitation::kind gives for free.
 *
 * The underlying type is fixed at uint32_t so a value from a newer producer (a kind this build
 * does not enumerate) is a well-defined enum value here rather than undefined behaviour; callers
 * must therefore never assume a parsed value is one of the enumerators below -- ask
 * isKnownSharedInvitationKind() first.
 *
 * NEVER renumber: this is a wire contract shared with .inv files and QR codes already in the
 * wild. Reserving a value here is deliberately separate from supporting it -- see
 * maxSharedInvitationVersion().
 */
enum class SharedInvitationKind : uint32_t
{
    Character=0,   //!< payload is `invitation` in shared_invitation::invitation (field 2)
    GroupChat=1    //!< value reserved only; payload unit and handling are not implemented yet,
                   //!< which maxSharedInvitationVersion() reports by returning 0 for it
};

//! Whether @a kind is a value this build enumerates at all -- false for anything a newer producer
//! invents. Distinct from "supported": see maxSharedInvitationVersion().
constexpr bool isKnownSharedInvitationKind(SharedInvitationKind kind) noexcept
{
    switch (kind)
    {
        case SharedInvitationKind::Character:
        case SharedInvitationKind::GroupChat:
            return true;
    }
    return false;
}

/**
 * @brief Highest shared_invitation::version this build can actually handle for @a kind.
 *
 * Returns 0 when this build cannot handle the kind at all -- either because it does not
 * enumerate it, or (GroupChat today) because the value is reserved in the wire contract but its
 * payload unit and flow are not implemented yet. Same "0 means this build doesn't know it"
 * convention the chat-message side uses for msg_type.
 *
 * Forward-compat policy is a hard block: a version above what is returned here is treated exactly
 * like an unknown kind, never parsed partially.
 */
constexpr uint32_t maxSharedInvitationVersion(SharedInvitationKind kind) noexcept
{
    switch (kind)
    {
        case SharedInvitationKind::Character: return 1;
        case SharedInvitationKind::GroupChat: return 0; //!< reserved, not implemented yet
    }
    return 0;
}

enum class InvitationPublishMode
{
    Public,
    Link,
    Qrcode,
    File
};

enum class InvitationReuseMode
{
    Unlimited,
    Once
};

enum class InvitationExpiration
{
    Never,
    Day,
    Week,
    Month,
    ThreeMonth,
    HalfYear,
    Year
};

inline common::DateTime invitationExpiration(InvitationExpiration expiration)
{
    auto dt=common::DateTime::currentUtc();

    switch (expiration)
    {
        case(InvitationExpiration::Never):
        {
            return common::DateTime{};
        }
        break;

        case(InvitationExpiration::Day):
        {
            dt.addDays(1);
        }
        break;

        case(InvitationExpiration::Week):
        {
            dt.addDays(7);
        }
        break;

        case(InvitationExpiration::Month):
        {
            dt.addMonths(1);
        }
        break;

        case(InvitationExpiration::ThreeMonth):
        {
            dt.addMonths(3);
        }
        break;

        case(InvitationExpiration::HalfYear):
        {
            dt.addMonths(6);
        }
        break;

        case(InvitationExpiration::Year):
        {
            dt.addYears(1);
        }
        break;
    }

    return dt;
}

//! Invitation object
HDU_UNIT_WITH(invitation,(
        HDU_BASE(HATN_DB_NAMESPACE::object) //!< Inherits from base object
    ),
    HDU_FIELD(guid,guid::TYPE,1) //!< Character's guid
    HDU_REPEATED_FIELD(hosts,server_host::TYPE,2) //!< Immediate routing information to find the character bypassing guid lookups
    HDU_REPEATED_FIELD(pubkeys,public_key::TYPE,3) //!< Public keys of the user, can be multiple, e.g. one for communication and another for notifications
    HDU_FIELD(expiration,TYPE_DATETIME,10) //!< Expiration of this invitation
    HDU_FIELD(reuse,HDU_TYPE_ENUM(InvitationReuseMode),11) //!< Mode of invitation reusing
)

/**
 * @brief Envelope actually written to a .inv file / encoded into a QR code.
 *
 * Extension rules, which every producer and consumer must follow:
 *
 *  - `prefix` is always InvitationPrefix, for EVERY kind. It is the cheap discriminator that lets
 *    a reader answer "this is an invitation, just not one I understand" instead of "this is not an
 *    invitation" -- namespacing it per kind would throw that away. See classifySharedInvitation().
 *  - `invitation` (field 2) is the Character-kind payload slot and NOTHING else. Every other kind
 *    puts its payload in `payload` (field 5) and leaves field 2 unset, so a reader that predates
 *    that kind fails cleanly on the "invitation field is empty" check it already performs, rather
 *    than silently mis-parsing a foreign unit as a contact invitation (the deserializer skips
 *    unknown tags, so it would NOT error on its own).
 *  - `kind`/`version` are NOT stamped for a plain Character invitation: their defaults already say
 *    exactly that, and leaving them off keeps existing .inv files and QR codes byte-identical in
 *    shape (QR density matters -- see QRCODE_SIZE_LIMIT_EXCEEDED).
 *  - `encryptable_object` is kind-agnostic (plain subunit or a passphrase-encrypted CryptContainer),
 *    so the protection-code wrapper is reused verbatim by every future kind rather than each
 *    inventing its own.
 */
HDU_UNIT(shared_invitation,
    HDU_FIELD(prefix,TYPE_STRING,1)
    HDU_FIELD(invitation,encryptable_object::TYPE,2) //!< Character kind ONLY, see above
    HDU_FIELD(kind,HDU_TYPE_ENUM(SharedInvitationKind),3,false,SharedInvitationKind::Character)
    HDU_FIELD(version,TYPE_UINT32,4,false,1) //!< payload version WITHIN `kind`, see maxSharedInvitationVersion()
    HDU_FIELD(payload,encryptable_object::TYPE,5) //!< payload of every kind except Character
)

HDU_UNIT(invitation_state,
    HDU_FIELD(inactive,TYPE_BOOL,20)
    HDU_FIELD(revoked,TYPE_BOOL,21)
    HDU_FIELD(use_count,TYPE_UINT32,22)
)

HDU_UNIT_WITH(client_invitation,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(invitation_state),HDU_BASE(at_server)),
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(invitation,invitation::TYPE,3)
    HDU_FIELD(mode,HDU_TYPE_ENUM(InvitationPublishMode),4)
    HDU_FIELD(protection_code,TYPE_STRING,5)
    HDU_FIELD(private_link,uri::TYPE,6)
    HDU_FIELD(full_username,uri::TYPE,7)
    HDU_FIELD(character,TYPE_STRING,8)
)

HDU_UNIT_WITH(invitation_parameters,(HDU_BASE(HATN_DB_NAMESPACE::with_expire)),
    HDU_FIELD(private_oid,TYPE_OBJECT_ID,1)
    HDU_FIELD(name,encryptable_string::TYPE,2)
    HDU_FIELD(password,encryptable_string::TYPE,3)
    HDU_FIELD(mode,HDU_TYPE_ENUM(InvitationPublishMode),4)
    HDU_FIELD(reuse,HDU_TYPE_ENUM(InvitationReuseMode),5,false,InvitationReuseMode::Unlimited)
)

HDU_UNIT_WITH(server_invitation_db,(HDU_BASE(HATN_DB_NAMESPACE::object),
                                     HDU_BASE(with_user),
                                     HDU_BASE(with_user_character),
                                     HDU_BASE(invitation_state)
                                    ),
    HDU_FIELD(parameters,invitation_parameters::TYPE,1)
    HDU_FIELD(content,TYPE_BYTES,2)
)

HDU_UNIT_WITH(server_invitation_register_response,(HDU_BASE(at_server),HDU_BASE(with_uri)),
    HDU_FIELD(invitation_domain,TYPE_STRING,1)
)

HDU_UNIT_WITH(server_invitation_publish_request,(HDU_BASE(at_server)),
    HDU_FIELD(content,TYPE_BYTES,1)
    HDU_FIELD(mode,TYPE_INT32,2)
)

HDU_UNIT_WITH(invitation_code,(HDU_BASE(username_reference),HDU_BASE(HATN_DB_NAMESPACE::with_expire)),)

constexpr const char* USER_REFERENCE_SCHEMA_USERNAME="username";
constexpr const char* USER_REFERENCE_SCHEMA_LINK="link";

HDU_UNIT_WITH(find_user_reference,(HDU_BASE(with_username),HDU_BASE(with_user_character)),
    HDU_FIELD(schema,TYPE_STRING,1,false,USER_REFERENCE_SCHEMA_USERNAME)
)

HDU_UNIT(find_user_reference_response,
    HDU_FIELD(invitation,TYPE_BYTES,1)
)

HDU_UNIT(ivitation_info,
    HDU_FIELD(parsed_invitation,invitation::TYPE,1)
    HDU_FIELD(user_info,user_character_public_sync::TYPE,2)
)

//! Parsed invitation and character info for parsed invitation
HDU_UNIT(character_for_invitation,
    HDU_FIELD(invitation,invitation::TYPE,1) //!< Invitation
    HDU_FIELD(character,global_character::TYPE,2) //!< Character info
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELINVITATION_H
