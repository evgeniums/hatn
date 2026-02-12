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

constexpr const char* UserCharacterType="character";

enum class UserCharacterSectionType
{
    Public=0,
    Private=1
};

enum class UserCharacterPubSection : uint32_t
{
    Avatar,
    Username,
    Name,
    Employment,    
    Addresses,
    PublicNotes,

    END=32
};

enum class UserCharacterPrivSection : uint32_t
{
    PrivateNotes,
    Notifications,
    ReadOnlyPubSections,
    ReadOnlyPrivSections,
    Sharing,

    END=32
};

struct UserCharacterPubSectionTraits
{
    using MaskType=uint32_t;
    using Feature=UserCharacterPubSection;
};

using UserCharacterPubSectionFeature=common::FeatureSet<UserCharacterPubSectionTraits>;
using UserCharacterPubSections=UserCharacterPubSectionFeature::Features;

struct UserCharacterPrivSectionTraits
{
    using MaskType=uint32_t;
    using Feature=UserCharacterPrivSection;
};

using UserCharacterPrivSectionFeature=common::FeatureSet<UserCharacterPrivSectionTraits>;
using UserCharacterPruvSections=UserCharacterPrivSectionFeature::Features;

namespace avatar_object=topic_object;

HDU_UNIT_WITH(user_character_public,(HDU_BASE(with_guid),
                                      HDU_BASE(with_name),
                                      HDU_BASE(with_username),
                                      HDU_BASE(with_revision)),
    HDU_FIELD(avatar,topic_object::TYPE,1)
    HDU_FIELD(notes,with_string::TYPE,4)
    HDU_FIELD(addresses,with_addresses::TYPE,5)
    HDU_FIELD(employment,employment::TYPE,6)
)

HDU_UNIT_WITH(user_character_private,(HDU_BASE(with_revision)),
    HDU_FIELD(private_notes,encryptable_string::TYPE,1)
    HDU_FIELD(shared_from,with_user_character::TYPE,2)
    HDU_FIELD(notifications,notifications::TYPE,3)
    HDU_FIELD(read_only_pub_sections,TYPE_UINT32,4)
    HDU_FIELD(read_only_priv_sections,TYPE_UINT32,5)
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

HDU_UNIT_WITH(user_character_public_sync,(HDU_BASE(with_user_character)),
    HDU_FIELD(public_data,user_character_public::TYPE,1)
)

HDU_UNIT_WITH(global_character,(HDU_BASE(global_object)),
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

HDU_UNIT_WITH(character,(HDU_BASE(user_character_full),HDU_BASE(at_server),HDU_BASE(with_uid)),
              )

HDU_UNIT(characters,
         HDU_REPEATED_FIELD(items,character::TYPE,1)
         HDU_FIELD(default_character,TYPE_OBJECT_ID,2)
         )

HDU_UNIT_WITH(update_character,(HDU_BASE(oid_key),HDU_BASE(with_revision)),
    HDU_FIELD(section_type,HDU_TYPE_ENUM(UserCharacterSectionType),1)
    HDU_FIELD(section,TYPE_UINT32,2)
    HDU_FIELD(content,TYPE_DATAUNIT,3)
)

HDU_UNIT(update_character_resp,
    HDU_FIELD(character,character::TYPE,1)
    HDU_FIELD(revision_before,TYPE_OBJECT_ID,2)
)

HDU_UNIT_WITH(get_public_character_info,(HDU_BASE(with_user_character)),
    HDU_FIELD(by_character,TYPE_OBJECT_ID,1)
)

HDU_UNIT_WITH(get_global_character,(HDU_BASE(with_uid)),
    HDU_FIELD(by_subject,uid::TYPE,1)
)

template <typename CharacterT>
inline common::SharedPtr<username::managed> characterUserName(const CharacterT& character)
{
    auto pub=character.field(user_character_full::public_data).sharedValue();
    if (pub)
    {
        auto uname=pub->field(with_username::uname).sharedValue();
        return uname;
    }
    return common::SharedPtr<username::managed>{};
}

template <typename T1, typename T2>
bool userCharacterPubSectionsEqual(UserCharacterPubSection section, const T1& l, const T2& r)
{
    switch(section)
    {
        case (UserCharacterPubSection::Username):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(with_username::uname,l,r);
        }
        break;

        case (UserCharacterPubSection::Name):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(with_name::name,l,r);
        }
        break;

        case (UserCharacterPubSection::Employment):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_public::employment,l,r);
        }
        break;

        case (UserCharacterPubSection::Avatar):
        {
            return HATN_DATAUNIT_NAMESPACE::subunitsEqual(user_character_public::avatar,l,r);
        }
        break;

        case (UserCharacterPubSection::PublicNotes):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_public::notes,l,r);
        }
        break;

        case (UserCharacterPubSection::Addresses):
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
bool userCharacterPrivSectionsEqual(UserCharacterPrivSection section, const T1& l, const T2& r)
{
    switch(section)
    {
        case (UserCharacterPrivSection::PrivateNotes):
        {
            return encryptableStringFieldsEqual(user_character_private::private_notes,l,r);
        }
        break;

        case (UserCharacterPrivSection::ReadOnlyPubSections):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_private::read_only_pub_sections,l,r);
        }
        break;

        case (UserCharacterPrivSection::ReadOnlyPrivSections):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_private::read_only_priv_sections,l,r);
        }
        break;

        case (UserCharacterPrivSection::Notifications):
        {
            return HATN_DATAUNIT_NAMESPACE::fieldEqual(user_character_private::notifications,l,r);
        }
        break;

        case (UserCharacterPrivSection::Sharing):
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
bool userCharacterSectionsEqual(UserCharacterSectionType UserCharacterSectionType, uint32_t section, const T1& l, const T2& r)
{
    switch (UserCharacterSectionType)
    {
        case (UserCharacterSectionType::Public):
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
            return userCharacterPubSectionsEqual(UserCharacterPubSectionFeature::feature(section),&lPub.value(),&rPub.value());
        }
        break;

        case (UserCharacterSectionType::Private):
        {
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
            return userCharacterPrivSectionsEqual(UserCharacterPrivSectionFeature::feature(section),&lPriv.value(),&rPriv.value());
        }
        break;
    }

    return true;
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERUSERCHARACTER_H
