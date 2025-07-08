/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/encryptedtoken.ipp
  */

/****************************************************************************/

#ifndef HATNENCRYPTEDTOKEN_IPP
#define HATNENCRYPTEDTOKEN_IPP

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/serverapp/encryptedtoken.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename TokenT>
Error EncryptedToken::serializeToken(
        const TokenT& token,
        common::ByteArray& encryptedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    HATN_CTX_SCOPE("encrypttoken::serialize")

    // serialize token
    Error ec;
    du::WireBufSolid buf{factory};
    du::io::serialize(token,buf,ec);
    HATN_CTX_CHECK_EC_LOG_MSG(ec,"failed to serialize token")

    // encrypt token
    ec=encrypt(buf.mainContainer(),encryptedToken);
    HATN_CHECK_EC(ec)

    // done
    return OK;
}

//--------------------------------------------------------------------------

template <typename TokenT>
Result<common::SharedPtr<TokenT>> EncryptedToken::parseToken(
        const common::ByteArray& encryptedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    HATN_CTX_SCOPE("encrypttoken::parse")

    // decrypt token
    du::WireBufSolid buf{factory};
    auto ec=decrypt(encryptedToken,*buf.mainContainer());
    HATN_CTX_CHECK_EC_LOG_MSG(ec,"failed to decrypt token")

    // deserialize token
    auto token=factory->createObject<TokenT>();
    du::io::deserialize(*token,buf,ec);
    HATN_CTX_CHECK_EC_LOG_MSG(ec,"failed to deserialize token")

    // done
    return token;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNENCRYPTEDTOKEN_IPP
