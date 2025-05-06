/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file api/client/testservicedb.h
  */

/****************************************************************************/

#ifndef HATNCLIENTTESSERVICEDB_H
#define HATNCLIENTTESSERVICEDB_H

#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/app/appdefs.h>

#include <hatn/api/client/clientbridge.h>
#include <hatn/api/client/bridgeappcontext.h>

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

class TestMethodOpenDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Method
{
    public:

        TestMethodOpenDb() : Method("open_db")
        {}

        virtual void exec(
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_CLIENT_BRIDGE_NAMESPACE::Context> ctx,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Request request,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Callback callback
        ) override;
};

class TestMethodDestroyDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Method
{
    public:

        TestMethodDestroyDb() : Method("destroy_db")
        {}

        virtual void exec(
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_CLIENT_BRIDGE_NAMESPACE::Context> ctx,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Request request,
            HATN_API_CLIENT_BRIDGE_NAMESPACE::Callback callback
        ) override;
};

class TestServiceDb : public HATN_API_CLIENT_BRIDGE_NAMESPACE::Service
{
    public:

        TestServiceDb(HATN_APP_NAMESPACE::App* app);
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

#endif // HATNCLIENTTESSERVICEDB_H
