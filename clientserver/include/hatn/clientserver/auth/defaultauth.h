/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/auth/defaultauth.h
  */

/****************************************************************************/

#ifndef HATNDEFAULTAUTH_H
#define HATNDEFAULTAUTH_H

#include <hatn/api/service.h>
#include <hatn/api/authprotocol.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

class HATN_CLIENT_SERVER_EXPORT DefaultAuthService
{
    public:

        static const api::Service& instance();
};

class HATN_CLIENT_SERVER_EXPORT DefaultAuthSessionProtocol
{
    public:

        static const api::AuthProtocol& instance();
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNDEFAULTAUTH_H
