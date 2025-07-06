/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/defaultauth.cpp
  *
  */

#include <hatn/api/authunit.h>

#include <hatn/clientserver/auth/authprotocol.h>
#include <hatn/clientserver/auth/defaultauth.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

const api::Service& DefaultAuthService::instance()
{
    static api::Service inst{AuthServiceName,AuthServiceVersion};
    return inst;
}

//--------------------------------------------------------------------------

const api::AuthProtocol& DefaultAuthSessionProtocol::instance()
{
    static api::AuthProtocol inst{api::AuthTokenSessionProtocol,api::AuthTokenSessionProtocolVersion};
    return inst;
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
