/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/sessiontoken.cpp
  */

/****************************************************************************/

#include <hatn/clientserver/clientservererror.h>

#include <hatn/serverapp/sessiontoken.h>

#include <hatn/serverapp/ipp/encryptedtoken.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

Error SessionToken::init(
        std::string currentTag,        
        TokenHandlers tagTokenHandlers
    )
{
    m_currentTag=std::move(currentTag);    
    m_tagTokenHandlers=std::move(tagTokenHandlers);

    auto it=m_tagTokenHandlers.find(m_currentTag);
    if (it==m_tagTokenHandlers.end())
    {
        return common::genericError(fmt::format(_TR("current tag \"{}\" not found in session tokens",m_currentTag)),common::CommonError::CONFIGURATION_ERROR);
    }

    return OK;
}

//--------------------------------------------------------------------------

Result<SessionToken::Tokens> SessionToken::makeToken(
        const session::type* session,
        auth_token::TokenType tokenType,
        uint32_t ttlSecs,
        const db::Topic& topic,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    Tokens tokens;

    // create session token
    tokens.sessionToken=factory->createObject<auth_token::managed>();

    // fill session token
    tokens.sessionToken->field(auth_token::id).mutableValue()->generate();
    tokens.sessionToken->field(auth_token::created_at).mutableValue()->loadCurrentUtc();
    tokens.sessionToken->setFieldValue(auth_token::session,session->fieldValue(db::object::_id));
    tokens.sessionToken->setFieldValue(auth_token::session_created_at,session->fieldValue(db::object::created_at));
    tokens.sessionToken->setFieldValue(auth_token::login,session->fieldValue(session::login));
    tokens.sessionToken->setFieldValue(auth_token::username,session->fieldValue(session::username));
    tokens.sessionToken->setFieldValue(auth_token::topic,topic.topic());
    tokens.sessionToken->setFieldValue(auth_token::token_type,tokenType);
    tokens.sessionToken->setFieldValue(auth_token::expire,tokens.sessionToken->field(auth_token::created_at).value());
    tokens.sessionToken->field(auth_token::expire).mutableValue()->addSeconds(static_cast<int>(ttlSecs));

    // create client token
    tokens.clientToken=factory->createObject<HATN_CLIENT_SERVER_NAMESPACE::auth_token::managed>();
    auto& tokenField=tokens.clientToken->field(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::token);
    auto* tokenBuf=tokenField.buf(true);
    tokens.clientToken->setFieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::tag,m_currentTag);
    tokens.clientToken->setFieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_token::expire,tokens.sessionToken->field(auth_token::expire).value());

    // serialize session token
    auto ec=m_currentTokenHandler->serializeToken(*tokens.sessionToken,*tokenBuf,factory);
    if (ec)
    {
        //! @todo critical: chain and log errors
        return ec;
    }

    // done
    return tokens;
}

//--------------------------------------------------------------------------

Result<common::SharedPtr<auth_token::managed>> SessionToken::parseToken(
        const HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::managed* clientToken,
        auth_token::TokenType tokenType,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    // find handler for tag
    EncryptedToken* handler=nullptr;
    if (clientToken->fieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::tag)==m_currentTag)
    {
        handler=m_currentTokenHandler.get();
    }
    else
    {
        auto it=m_tagTokenHandlers.find(clientToken->fieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::tag));
        if (it==m_tagTokenHandlers.end())
        {
            return HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_TOKEN_TAG_INVALID);
        }
        handler=it->second.get();
    }

    // parse token
    const auto& tokenField=clientToken->field(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::token);
    const auto* tokenBuf=tokenField.buf();
    auto tokenR=handler->parseToken<auth_token::managed>(*tokenBuf,factory);
    if (tokenR)
    {
        //! @todo critical: chain and log errors
        return tokenR.takeError();
    }

    // check token type
    if (tokenType!=tokenR.value()->fieldValue(auth_token::token_type))
    {
        return HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_TOKEN_INVALID_TYPE);
    }

    // check if token expired
    auto now=common::DateTime::currentUtc();
    if (now.after(tokenR.value()->fieldValue(auth_token::expire)))
    {
        return HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_TOKEN_EXPIRED);
    }

    // done
    return tokenR;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END
