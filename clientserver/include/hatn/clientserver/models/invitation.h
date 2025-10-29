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

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/encrypted.h>
#include <hatn/clientserver/models/serverhost.h>
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/models/usercharacter.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

enum class InvitationPublishMode
{
    Public,
    Link,
    File,
    Qrcode
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

HDU_UNIT_WITH(invitation,(HDU_BASE(HATN_DB_NAMESPACE::object)),
    HDU_FIELD(guid,guid::TYPE,1)
    HDU_REPEATED_FIELD(hosts,server_host::TYPE,2)
    HDU_REPEATED_FIELD(pubkeys,public_key::TYPE,3)
    HDU_FIELD(expiration,TYPE_DATETIME,10)
    HDU_FIELD(reuse,HDU_TYPE_ENUM(InvitationReuseMode),11)
)

HDU_UNIT(shared_invitation,
    HDU_FIELD(invitation,encryptable_object::TYPE,1)
    HDU_FIELD(signature,invitation::TYPE,2)
)

HDU_UNIT(invitation_state,
    HDU_FIELD(inactive,TYPE_BOOL,20)
    HDU_FIELD(revoked,TYPE_BOOL,21)
    HDU_FIELD(use_count,TYPE_UINT32,22)
)

HDU_UNIT_WITH(client_invitation,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(invitation_state)),
    HDU_FIELD(name,TYPE_STRING,2)
    HDU_FIELD(invitation,invitation::TYPE,3)
    HDU_FIELD(mode,HDU_TYPE_ENUM(InvitationPublishMode),4)
    HDU_FIELD(protection_code,TYPE_STRING,5)
    HDU_FIELD(private_link,uri::TYPE,6)
)

HDU_UNIT_WITH(server_invitation,(HDU_BASE(HATN_DB_NAMESPACE::object),
                                  HDU_BASE(with_user),
                                  HDU_BASE(with_user_character),
                                  HDU_BASE(invitation_state)),
    HDU_FIELD(private_oid,TYPE_OBJECT_ID,1)
    HDU_FIELD(name,encryptable_string::TYPE,2)
    HDU_FIELD(invitation,shared_invitation::TYPE,3)
    HDU_FIELD(expiration,TYPE_DATETIME,10)
    HDU_FIELD(reuse,HDU_TYPE_ENUM(InvitationReuseMode),11)
)

HDU_UNIT_WITH(temporary_invitation,(HDU_BASE(HATN_DB_NAMESPACE::object),HDU_BASE(with_user),HDU_BASE(with_user_character)),
    HDU_FIELD(direct_id,TYPE_STRING,1)
    HDU_FIELD(invitation_oid,TYPE_OBJECT_ID,2)
    HDU_FIELD(topic,TYPE_STRING,3)
)

HDU_UNIT_WITH(create_invitation_response,(HDU_BASE(with_uri)),
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELINVITATION_H
