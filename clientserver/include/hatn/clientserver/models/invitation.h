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

constexpr const char* InvitationPrefix="HINV";

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

HDU_UNIT(shared_invitation,
    HDU_FIELD(prefix,TYPE_STRING,1)
    HDU_FIELD(invitation,encryptable_object::TYPE,2)
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
