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

class ClientAuthProtocol : public api::AuthProtocol
{
    public:

        ClientAuthProtocol(lib::string_view name,
                           VersionType version,
                           std::shared_ptr<api::Service> service={}
                           )
                    : api::AuthProtocol(name,version),
                      m_service(std::move(service))
        {}

        const api::Service* service() const noexcept
        {
            return m_service.get();
        }

        auto serviceShared() const
        {
            return m_service;
        }

        void setService(std::shared_ptr<api::Service> service)
        {
            m_service=std::move(service);
        }

    private:

        std::shared_ptr<api::Service> m_service;
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTAUTHPROTOCOL_H
