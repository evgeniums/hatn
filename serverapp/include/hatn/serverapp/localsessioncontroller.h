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

HDU_UNIT(session_token,
    HDU_FIELD(tag,TYPE_STRING,1,true)
    HDU_FIELD(secret,TYPE_STRING,2,true)
    HDU_FIELD(cipher_suite,TYPE_STRING,3)
)

HDU_UNIT(session_config,
    HDU_FIELD(current_tag,TYPE_STRING,1,true)
    HDU_FIELD(session_token_ttl_secs,TYPE_UINT32,2,false,21600)
    HDU_FIELD(refresh_token_ttl_secs,TYPE_UINT32,3,false,2592000)
    HDU_REPEATED_FIELD(tokens,session_token::TYPE,4,true)
)

class HATN_SERVERAPP_EXPORT LocalSessionControllerBase : public HATN_BASE_NAMESPACE::ConfigObject<session_config::type>
{
    public:

        Error init(const crypt::CipherSuites* suites);

        const SessionToken& tokenHandler() const noexcept
        {
            return m_tokenHandler;
        }

    private:

        SessionToken m_tokenHandler;
};

template <typename ContextTraits>
class LocalSessionController : public LocalSessionControllerBase
{
    public:

        using Context=typename ContextTraits::Context;

        template <typename CallbackT>
        void createSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& login,
            db::Topic topic
        ) const;

        template <typename CallbackT>
        void checkSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::managed> sessionContent
        ) const;

        template <typename CallbackT>
        void refreshSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_refresh::managed> message
        ) const;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_H
