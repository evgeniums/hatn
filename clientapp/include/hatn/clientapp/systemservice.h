/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/systemservice.h
  *
  */

/****************************************************************************/

#ifndef HATNSYSTEMSERVICE_H
#define HATNSYSTEMSERVICE_H

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/bridgeappcontext.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class HATN_CLIENTAPP_EXPORT SystemService : public Service
{
    public:

        constexpr static const char* Name="system";

        SystemService(ClientApp* app);

        int controller() const
        {
            return 0;
        }
};

using SystemServiceMethod=ServiceMethod<SystemService>;

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNSYSTEMSERVICE_H
