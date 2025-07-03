/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/plaintcpclientwithauth.сpp
  *
  */

#include <hatn/clientserver/plaintcpclientwithauth.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/clientrequest.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/session.ipp>
#include <hatn/api/ipp/makeapierror.ipp>

#include <hatn/clientserver/ipp/clientsession.ipp>
#include <hatn/clientserver/ipp/clientauthprotocolsharedsecret.ipp>
#include <hatn/clientserver/ipp/clientwithauth.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

template class HATN_CLIENT_SERVER_EXPORT ClientWithAuthT<clientapi::PlainTcpRouter,PlainTcpClientWithAuth::RequestContext,PlainTcpClientWithAuth::MessageBuf,PlainTcpClientWithAuth::RequestUnit,ClientAuthProtocolSharedSecret>;

HATN_CLIENT_SERVER_NAMESPACE_END

HATN_API_NAMESPACE_BEGIN

namespace client
{
template class HATN_CLIENT_SERVER_EXPORT Client<PlainTcpRouter,SessionWrapper<HATN_CLIENT_SERVER_NAMESPACE::ClientSessionSharedSecret>>;
}

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::PlainTcpClientWithAuth,PlainTcpClientWithAuth)
HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::PlainTcpClientWithAuth::Client,PlainTcpClientWithAuthClient)
HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::PlainTcpClientWithAuth::Session,PlainTcpClientWithAuthSession)
