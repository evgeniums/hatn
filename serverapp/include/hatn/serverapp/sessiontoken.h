/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/sessiontoken.h
  */

/****************************************************************************/

#ifndef HATNLOCALSESSIONTOKEN_H
#define HATNLOCALSESSIONTOKEN_H

#include <hatn/common/flatmap.h>

#include <hatn/db/topic.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>
#include <hatn/serverapp/auth/authtokens.h>
#include <hatn/serverapp/sessiondbmodels.h>
#include <hatn/serverapp/encryptedtoken.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class HATN_SERVERAPP_EXPORT SessionToken
{
    public:

        using TokenHandlers=common::FlatMap<std::string,std::shared_ptr<EncryptedToken>,std::less<>>;

        Error init(
            std::string currentTag,
            TokenHandlers tagTokenHandlers
        );

        struct Tokens
        {
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_token::managed> clientToken;
            common::SharedPtr<auth_token::managed> sessionToken;
        };

        Result<Tokens> makeToken(
            const session::type* session,
            auth_token::TokenType tokenType,
            uint32_t ttlSecs,
            const db::Topic& topic,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

        Result<common::SharedPtr<auth_token::managed>> parseToken(
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_token::managed> clientToken,
            auth_token::TokenType tokenType,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const;

    private:

        TokenHandlers m_tagTokenHandlers;
        std::shared_ptr<EncryptedToken> m_currentTokenHandler;
        std::string m_currentTag;
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONTOKEN_H
