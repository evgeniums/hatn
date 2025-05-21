/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminclient/userserviceconstants.h
  */

/****************************************************************************/

#ifndef HATNUSERSERVICECONSTANTS_H
#define HATNUSERSERVICECONSTANTS_H

#include <hatn/adminclient/adminclient.h>

HATN_ADMIN_CLIENT_NAMESPACE_BEGIN

struct UserServiceConstants
{
    constexpr static const char* Service="user";

    constexpr static const char* AddUser="add_user";
};

HATN_ADMIN_CLIENT_NAMESPACE_END

#endif // HATNUSERSERVICECONSTANTS_H
