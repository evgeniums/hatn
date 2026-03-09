/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientsessionsharedsecret.сpp
  *
  */

#include <hatn/clientserver/auth/authprotocol.h>
#include <hatn/clientserver/auth/clientsessionsharedsecret.h>

#include <hatn/api/ipp/session.ipp>
#include <hatn/clientserver/ipp/clientsession.ipp>
#include <hatn/clientserver/ipp/clientauthprotocolsharedsecret.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

const api::Method& hssLogin()
{
    static api::Method method{AuthHssLoginMethodName};
    return method;
}

template class HATN_CLIENT_SERVER_EXPORT ClientSessionTraits<ClientAuthProtocolSharedSecret>;

HATN_CLIENT_SERVER_NAMESPACE_END

HATN_API_NAMESPACE_BEGIN

namespace client
{

template class HATN_CLIENT_SERVER_EXPORT Session<HATN_CLIENT_SERVER_NAMESPACE::ClientSessionTraits<HATN_CLIENT_SERVER_NAMESPACE::ClientAuthProtocolSharedSecret>>;

}

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::ClientSessionSharedSecret,ClientSessionSharedSecret)
