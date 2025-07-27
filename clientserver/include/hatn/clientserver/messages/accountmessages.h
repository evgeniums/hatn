/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/messages/accountmessages.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERACCOUNTMESSAGES_H
#define HATNCLIENTSERVERACCOUNTMESSAGES_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/authunit.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/loginprofile.h>
#include <hatn/clientserver/models/usercharacter.h>
#include <hatn/clientserver/models/serverroute.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

namespace messages {

HDU_UNIT(activate_account,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(token_tag,TYPE_STRING,2)
    HDU_FIELD(password,TYPE_STRING,3)
)

HDU_UNIT(account_login,
    HDU_FIELD(login,login_profile::TYPE,1)
    HDU_REPEATED_FIELD(server_route,server_route_hop::TYPE,2)
)

HDU_UNIT(activate_account_response,
    HDU_FIELD(login,account_login::TYPE,1)
    HDU_FIELD(character,user_character::TYPE,2)
)

HDU_UNIT(commit_account_activation,
    HDU_FIELD(token,TYPE_BYTES,1)
    HDU_FIELD(token_tag,TYPE_STRING,2)
)

}

namespace methods {

constexpr const char* AccountServiceName="account";
constexpr const uint32_t AccountServiceVersion=1;
constexpr const char* AccountMethodActivate="activate";
constexpr const char* AccountMethodCommitActivation="commit_activation";

}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERACCOUNTMESSAGES_H
