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
    // serialize token
    Error ec;
    du::WireBufSolid buf{factory};
    du::io::serialize(token,buf,ec);
    if (ec)
    {
        //! @todo critical: chain errors
        return ec;
    }

    // encrypt token
    return encrypt(buf.mainContainer(),encryptedToken);
}

//--------------------------------------------------------------------------

template <typename TokenT>
Result<common::SharedPtr<TokenT>> EncryptedToken::parseToken(
        const common::ByteArray& encryptedToken,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    // decrypt token
    du::WireBufSolid buf{factory};
    auto ec=decrypt(encryptedToken,*buf.mainContainer());
    if (ec)
    {
        //! @todo critical: chain and log error
        return ec;
    }

    // deserialize token
    auto token=factory->createObject<TokenT>();
    du::io::deserialize(*token,buf,ec);
    if (ec)
    {
        //! @todo critical: chain and log error        
        return ec;
    }

    // done
    return token;
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNENCRYPTEDTOKEN_IPP
