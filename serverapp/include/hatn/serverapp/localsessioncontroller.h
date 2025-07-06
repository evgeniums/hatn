/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/localsessioncontroller.h
  */

/****************************************************************************/

#ifndef HATNLOCALSESSIONCONTROLLER_H
#define HATNLOCALSESSIONCONTROLLER_H

#include <hatn/base/configobject.h>

#include <hatn/dataunit/objectid.h>
#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/objectid.h>

#include <hatn/crypt/crypt.h>

#include <hatn/db/topic.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/sessiontoken.h>

HATN_CRYPT_NAMESPACE_BEGIN
class CipherSuites;
HATN_CRYPT_NAMESPACE_END

HATN_SERVERAPP_NAMESPACE_BEGIN

class SessionDbModels;

HDU_UNIT(session_token,
    HDU_FIELD(tag,TYPE_STRING,1,true)
    HDU_FIELD(secret,TYPE_STRING,2,true)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
)

HDU_UNIT(session_config,
    HDU_FIELD(current_tag,TYPE_STRING,1,true)
    HDU_FIELD(session_token_ttl_secs,TYPE_UINT32,2,false,21600)
    HDU_FIELD(refresh_token_ttl_secs,TYPE_UINT32,3,false,2592000)
    HDU_FIELD(session_ttl_days,TYPE_UINT32,4,false,365)
    HDU_REPEATED_FIELD(tokens,session_token::TYPE,5,true)
)

class HATN_SERVERAPP_EXPORT LocalSessionControllerBase : public HATN_BASE_NAMESPACE::ConfigObject<session_config::type>
{
    public:

        LocalSessionControllerBase(std::shared_ptr<SessionDbModels> sessionDbModels);
        ~LocalSessionControllerBase();

        LocalSessionControllerBase(const LocalSessionControllerBase&)=default;
        LocalSessionControllerBase(LocalSessionControllerBase&&)=default;
        LocalSessionControllerBase& operator=(const LocalSessionControllerBase&)=default;
        LocalSessionControllerBase& operator=(LocalSessionControllerBase&&)=default;

        Error init(const crypt::CipherSuites* suites);

        std::shared_ptr<SessionToken> tokenHandler() const
        {
            return m_tokenHandler;
        }

        std::shared_ptr<SessionDbModels> sessionDbModels() const noexcept
        {
            return m_sessionDbModels;
        }

    private:

        std::shared_ptr<SessionToken> m_tokenHandler;
        std::shared_ptr<SessionDbModels> m_sessionDbModels;
};

template <typename ContextTraits>
class LocalSessionController : public LocalSessionControllerBase
{
    public:

        using Context=typename ContextTraits::Context;

        using LocalSessionControllerBase::LocalSessionControllerBase;

        template <typename CallbackT>
        void createSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& login,
            lib::string_view username,
            db::Topic topic
        ) const;

        template <typename CallbackT>
        void updateSessionClient(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const common::SharedPtr<session::managed>& session,
            db::Topic topic
        ) const;

        template <typename CallbackT>
        void checkSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            common::SharedPtr<auth_with_token::managed> sessionContent,
            bool update=true
        ) const;

        template <typename CallbackT>
        void refreshSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            common::SharedPtr<auth_refresh::managed> message
        ) const;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_H
