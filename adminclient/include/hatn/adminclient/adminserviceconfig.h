/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminclient/adminserviceconfig.h
  */

/****************************************************************************/

#ifndef HATNADMINSERVICECONFIG_H
#define HATNADMINSERVICECONFIG_H

#include <hatn/adminclient/adminclient.h>

HATN_ADMIN_CLIENT_NAMESPACE_BEGIN

struct AdminServiceConfig
{
    constexpr static const char* Service="admin";

    constexpr static const char* AddAdmin="add";
};

HATN_ADMIN_CLIENT_NAMESPACE_END

#endif // HATNADMINSERVICECONFIG_H
