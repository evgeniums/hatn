/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/plaintcpclientwithauth.h
  */

/****************************************************************************/

#ifndef HATNPLAINTCPCLIENTWITHAUTH_H
#define HATNPLAINTCPCLIENTWITHAUTH_H

#include <hatn/api/client/plaintcprouter.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/clientwithauth.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

using PlainTcpClientWithAuth=ClientWithSharedSecretAuthT<clientapi::PlainTcpRouter>;
constexpr makeClientWithSharedSecretAuthContextT<clientapi::PlainTcpRouter> makePlainTcpClientWithAuthContext;

HATN_CLIENT_SERVER_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_CLIENT_SERVER_NAMESPACE::PlainTcpClientWithAuth,HATN_CLIENT_SERVER_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_CLIENT_SERVER_NAMESPACE::PlainTcpClientWithAuth::Client,HATN_CLIENT_SERVER_EXPORT)

#endif // HATNPLAINTCPCLIENTWITHAUTH_H
