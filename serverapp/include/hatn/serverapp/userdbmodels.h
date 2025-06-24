/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/userdbmodels.h
  */

/****************************************************************************/

#ifndef HATNUSERDBMODELS_H
#define HATNUSERDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/clientserver/models/user.h>
#include <hatn/clientserver/models/usercharacter.h>
#include <hatn/clientserver/models/loginprofile.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

HATN_DB_UNIQUE_INDEX(userPhoneIdx,user_profile::phone)
HATN_DB_UNIQUE_INDEX(userEmailIdx,user_profile::email)
HATN_DB_INDEX(userNameIdx,user_profile::name)
HATN_DB_UNIQUE_INDEX(userReferenceIdx,user::reference,user::reference_type)
HATN_DB_MODEL_PROTOTYPE(userModel,user,userNameIdx(),userPhoneIdx(),userEmailIdx(),userReferenceIdx())

HATN_DB_INDEX(withUserIdx,with_user::user,with_user::user_topic)
HATN_DB_INDEX(withUserCharacterIdx,with_user_character::user_character,with_user_character::user_character_topic)
HATN_DB_INDEX(withLoginProfileIdx,with_login_profile::login_profile,with_login_profile::login_profile_topic)

HATN_DB_MODEL_PROTOTYPE(userCharacterModel,user_character,withUserIdx())

HATN_DB_MODEL_PROTOTYPE(userCharacterShareModel,user_character_share,withUserIdx(),withUserCharacterIdx())

HATN_DB_INDEX(loginParameter1Idx,login_profile::parameter1)
HATN_DB_INDEX(loginParameter2Idx,login_profile::parameter2)
HATN_DB_MODEL_PROTOTYPE(loginProfileModel,login_profile,withUserIdx(),loginParameter1Idx(),loginParameter2Idx())

HATN_DB_MODEL_PROTOTYPE(userCharacterLoginModel,user_character_login,withUserCharacterIdx(),withLoginProfileIdx())

class UserDbModels : public db::ModelsWrapper
{
    public:

        UserDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& userModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::userModel);
        }

        const auto& userCharacterModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::userCharacterModel);
        }

        const auto& loginProfileModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::loginProfileModel);
        }

        const auto& userCharacterShareModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::userCharacterShareModel);
        }

        const auto& userCharacterLoginModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_SERVERAPP_NAMESPACE::userCharacterLoginModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return userModel();},
                [this](){return userCharacterModel();},
                [this](){return loginProfileModel();},
                [this](){return userCharacterShareModel();},
                [this](){return userCharacterLoginModel();}
            );
        }
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNUSERDBMODELS_H
