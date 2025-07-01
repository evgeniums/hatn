/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientsession.сpp
  *
  */

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/clientserver/auth/authprotocol.h>
#include <hatn/clientserver/auth/clientsession.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------
ClientSessionImpl::ClientSessionImpl(
        const common::pmr::AllocatorFactory* factory
    ) : common::pmr::WithFactory(factory),
        api::AuthProtocol(api::AuthTokenSessionProtocol,api::AuthTokenSessionProtocolVersion),
        api::WithService(std::make_shared<api::Service>(AuthServiceName,AuthServiceVersion))
{
}

//--------------------------------------------------------------------------

Error ClientSessionImpl::loadToken(common::SharedPtr<auth_token::shared_managed>& token, lib::string_view content) const
{
    token=factory()->createObject<auth_token::shared_managed>();

    du::WireBufSolid buf{content.data(),content.size(),true,factory()};

    Error ec;
    du::io::deserialize(*token,buf,ec);
    return ec;
}

//--------------------------------------------------------------------------

Error ClientSessionImpl::loadSessionToken(lib::string_view content)
{
    auto ec=loadToken(m_sessionToken,content);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to load session token")
    return OK;
}

//--------------------------------------------------------------------------

Error ClientSessionImpl::loadRefreshToken(lib::string_view content)
{
    auto ec=loadToken(m_refreshToken,content);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to load refresh token")
    return OK;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
