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
#include <hatn/clientserver/auth/defaultauth.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------
ClientSessionBase::ClientSessionBase(
        const common::pmr::AllocatorFactory* factory
    ) : common::pmr::WithFactory(factory),
        api::AuthProtocol(DefaultAuthSessionProtocol::instance()),
        api::WithService(std::make_shared<api::Service>(DefaultAuthService::instance()))
{
}

//--------------------------------------------------------------------------

Error ClientSessionBase::loadToken(common::SharedPtr<auth_token::managed>& token, lib::string_view content) const
{
    token=factory()->createObject<auth_token::managed>();

    token->setParseToSharedArrays(true);
    du::WireBufSolid buf{content.data(),content.size(),true,factory()};

    Error ec;
    du::io::deserialize(*token,buf,ec);
    return ec;
}

//--------------------------------------------------------------------------

Error ClientSessionBase::loadSessionToken(lib::string_view content)
{    
    auto ec=loadToken(m_sessionTokenContainer,content);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to load session token")
    return OK;
}

//--------------------------------------------------------------------------

Error ClientSessionBase::loadRefreshToken(lib::string_view content)
{
    auto ec=loadToken(m_refreshTokenContainer,content);
    HATN_CTX_CHECK_EC_MSG(ec,"failed to load refresh token")
    return OK;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
