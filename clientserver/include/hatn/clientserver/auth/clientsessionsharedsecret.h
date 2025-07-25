/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/clientsessionsharedsecret.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSESSIONSHAREDSECRET_H
#define HATNCLIENTSESSIONSHAREDSECRET_H

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/auth/clientsession.h>
#include <hatn/clientserver/auth/clientauthprotocolsharedsecret.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

using ClientSessionSharedSecret=ClientSession<ClientAuthProtocolSharedSecret>;
using ClientSessionSharedSecretContext=ClientSessionContext<ClientAuthProtocolSharedSecret>;

using ClientSessionHssWrapper=clientapi::SessionWrapper<ClientSessionSharedSecret,ClientSessionSharedSecretContext>;

struct allocateSessionSharedSecretContextT
{
    template <typename ...Args>
    auto operator () (
        const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<ClientSessionSharedSecret>& allocator,
        Args&&... args
        ) const
    {
        return clientapi::SessionWrapper<ClientSessionSharedSecret,ClientSessionSharedSecretContext>{
            HATN_COMMON_NAMESPACE::allocateTaskContextType<ClientSessionSharedSecretContext>(
                allocator,
                HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                    HATN_COMMON_NAMESPACE::subcontext()
                    )
                )
        };
    }
};
constexpr allocateSessionSharedSecretContextT allocateSessionSharedSecretContext{};

struct makeSessionSharedSecretContextT
{
    template <typename ...Args>
    auto operator () (
        Args&&... args
        ) const
    {
        return clientapi::SessionWrapper<ClientSessionSharedSecret,ClientSessionSharedSecretContext>{
            HATN_COMMON_NAMESPACE::makeTaskContextType<ClientSessionSharedSecretContext>(
                HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                    HATN_COMMON_NAMESPACE::subcontext()
                    )
                )
        };
    }
};
constexpr makeSessionSharedSecretContextT makeSessionSharedSecretContext{};

HATN_CLIENT_SERVER_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(
        HATN_CLIENT_SERVER_NAMESPACE::ClientSessionSharedSecret,
        HATN_CLIENT_SERVER_EXPORT
    )

#endif // HATNCLIENTSESSIONSHAREDSECRET_H
