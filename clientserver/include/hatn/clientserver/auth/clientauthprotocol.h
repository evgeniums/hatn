/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/clientauthprotocol.h
  */

/****************************************************************************/

#ifndef HATNCLIENTAUTHPROTOCOL_H
#define HATNCLIENTAUTHPROTOCOL_H

#include <hatn/common/objecttraits.h>

#include <hatn/api/authprotocol.h>
#include <hatn/api/service.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/authprotocol.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

namespace clientapi=HATN_API_NAMESPACE::client;
namespace api=HATN_API_NAMESPACE;

class ClientSessionImpl;

class ClientAuthProtocol : public api::AuthProtocol
{
    public:

        ClientAuthProtocol(lib::string_view name,
                           VersionType version
                           )
                    : api::AuthProtocol(name,version),
                      m_session(nullptr)
        {}

        ClientSessionImpl* session() const noexcept
        {
            return m_session;
        }

        void setSession(ClientSessionImpl* session) noexcept
        {
            m_session=session;
        }

    private:

        ClientSessionImpl* m_session;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTAUTHPROTOCOL_H
