/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/usercharacter.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERUSERCHARACTER_H
#define HATNCLIENTSERVERUSERCHARACTER_H

#include <hatn/common/featureset.h>

#include <hatn/dataunit/compare.h>

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/withuser.h>
#include <hatn/clientserver/models/withusercharacter.h>
#include <hatn/clientserver/models/withloginprofile.h>
#include <hatn/clientserver/models/encrypted.h>
#include <hatn/clientserver/models/oid.h>
#include <hatn/clientserver/models/revision.h>
#include <hatn/clientserver/models/name.h>
#include <hatn/clientserver/models/username.h>
#include <hatn/clientserver/models/addressitem.h>
#include <hatn/clientserver/models/employment.h>
#include <hatn/clientserver/models/notifications.h>
#include <hatn/clientserver/models/unreadcount.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

enum class UserCharacterSection : uint32_t
{
    Username,
    Name,
    Employment,
    Avatar,
    AddressItem,
    PublicNotes,

    PrivateNotes,    
    Notifications,
    ReadOnlySections,
    Sharing,

    END=32
};

struct UserCharacterSectionTraits
{
    using MaskType=uint32_t;
    using Feature=UserCharacterSection;
};

using UserCharacterSectionFeature=common::FeatureSet<UserCharacterSectionTraits>;
using UserCharacterSections=UserCharacterSectionFeature::Features;

constexpr std::initializer_list<UserCharacterSection> UserCharacterPubSections{
    UserCharacterSection::Username,
    UserCharacterSection::Name,
    UserCharacterSection::Employment,
    UserCharacterSection::Avatar,
    UserCharacterSection::AddressItem,
    UserCharacterSection::PublicNotes
};

constexpr std::initializer_list<UserCharacterSection> UserCharacterPrivSections{
    UserCharacterSection::PrivateNotes,
    UserCharacterSection::Notifications,
    UserCharacterSection::ReadOnlySections,
    UserCharacterSection::Sharing
};

inline bool isUserCharacterPublicSection(UserCharacterSection section)
{
    auto sections=UserCharacterSectionFeature::featureBit(section);
    return UserCharacterSectionFeature::hasFeatures(sections,UserCharacterPubSections);
}

HDU_UNIT_WITH(user_character_public,(HDU_BASE(with_name),HDU_BASE(with_username),HDU_BASE(with_revision)),
    HDU_FIELD(avatar,topic_object::TYPE,1)
    HDU_FIELD(notes,TYPE_STRING,4)
    HDU_REPEATED_FIELD(addresses,address_item::TYPE,5)
    HDU_FIELD(employment,employment::TYPE,6)
)

HDU_UNIT_WITH(user_character_private,(HDU_BASE(with_revision)),
    HDU_FIELD(encrypted_notes,encrypted::TYPE,1)
    HDU_FIELD(plain_notes,TYPE_STRING,2)
    HDU_FIELD(shared_from,with_user_character::TYPE,3)
    HDU_FIELD(notifications,notifications::TYPE,4)
    HDU_FIELD(read_only_secions,TYPE_UINT32,5)
)

HDU_UNIT_WITH(user_character_full,(HDU_BASE(db::object)),
     HDU_FIELD(public_data,user_character_public::TYPE,1)
     HDU_FIELD(private_data,user_character_private::TYPE,2)
)

HDU_UNIT_WITH(user_character_server_db,(HDU_BASE(with_user),HDU_BASE(user_character_full)),
    HDU_FIELD(server_blocked,TYPE_BOOL,30)
    HDU_FIELD(server_notes,TYPE_STRING,31)
    HDU_FIELD(server_permissions,TYPE_UINT64,32)
)

HDU_UNIT_WITH(user_character_public_sync,(HDU_BASE(oid)),
    HDU_FIELD(public_data,user_character_public::TYPE,1)
)

HDU_UNIT_WITH(user_character_private_sync,(HDU_BASE(user_character_full),HDU_BASE(unread_count))
)

HDU_UNIT_WITH(user_character_share,(HDU_BASE(db::object),HDU_BASE(with_user),HDU_BASE(with_user_character)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(access,TYPE_UINT32,2)
    HDU_FIELD(description,TYPE_STRING,3)
)

HDU_UNIT_WITH(user_character_login,(HDU_BASE(db::object),HDU_BASE(with_user_character),HDU_BASE(with_login_profile)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,3)
)

HDU_UNIT_WITH(character,(HDU_BASE(user_character_full),HDU_BASE(at_server)),
              )

HDU_UNIT(characters,
         HDU_REPEATED_FIELD(items,character::TYPE,1)
         HDU_FIELD(default_character,TYPE_OBJECT_ID,2)
         )

HDU_UNIT_WITH(update_character,(HDU_BASE(oid_key)),
              HDU_FIELD(section,HDU_TYPE_ENUM(UserCharacterSection),1)
              HDU_FIELD(content,TYPE_DATAUNIT,2)
              )

HDU_UNIT(update_character_resp,
         HDU_FIELD(character,character::TYPE,1)
         HDU_FIELD(section,HDU_TYPE_ENUM(UserCharacterSection),2)
         )

template <typename T1, typename T2>
bool userCharacterPublicSectionsEqual(UserCharacterSection section, const T1& l, const T2& r)
{
    switch(section)
    {
        case (UserCharacterSection::Username):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(with_username::uname,l,r);
        }
        break;

        case (UserCharacterSection::Name):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(with_name::name,l,r);
        }
        break;

        case (UserCharacterSection::Employment):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_public::employment,l,r);
        }
        break;

        case (UserCharacterSection::Avatar):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_public::avatar,l,r);
        }
        break;

        case (UserCharacterSection::PublicNotes):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_public::notes,l,r);
        }
        break;

        case (UserCharacterSection::AddressItem):
        {
            return HATN_DATAUNIT_NAMESPACE::repeatedSubunitsEqual(user_character_public::addresses,l,r);
        }
        break;

        default:
            break;
    }

    return true;
}

template <typename T1, typename T2>
bool userCharacterPrivateSectionsEqual(UserCharacterSection section, const T1& l, const T2& r)
{
    switch(section)
    {
        case (UserCharacterSection::PrivateNotes):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_private::plain_notes,l,r)
                    &&
                   HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_private::encrypted_notes,l,r)
                ;
        }
        break;

        case (UserCharacterSection::ReadOnlySections):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_private::read_only_secions,l,r);
        }
        break;

        case (UserCharacterSection::Notifications):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_private::notifications,l,r);
        }
        break;

        case (UserCharacterSection::Sharing):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_private::shared_from,l,r);
        }
        break;

        default:
            break;
    }

    return true;
}

template <typename T1, typename T2>
bool userCharacterSectionsEqual(UserCharacterSection section, const T1& l, const T2& r)
{
    if (isUserCharacterPublicSection(section))
    {
        auto lPub=l->field(user_character_full::private_data);
        auto rPub=r->field(user_character_full::private_data);
        if (!lPub.isSet())
        {
            if (!rPub.isSet())
            {
                return true;
            }
            return false;
        }
        return userCharacterPublicSectionsEqual(section,&lPub.value(),&rPub.value());
    }

    auto lPriv=l->field(user_character_full::private_data);
    auto rPriv=r->field(user_character_full::private_data);
    if (!lPriv.isSet())
    {
        if (!rPriv.isSet())
        {
            return true;
        }
        return false;
    }
    return userCharacterPrivateSectionsEqual(section,&lPriv.value(),&rPriv.value());
}


HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERUSERCHARACTER_H
