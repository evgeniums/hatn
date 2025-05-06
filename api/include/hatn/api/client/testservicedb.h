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

#include <hatn/api/api.h>

#include <hatn/api/client/clientbridge.h>
#include <hatn/api/client/bridgeappcontext.h>

HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN

class HATN_API_EXPORT TestMethodOpenDb : public Method
{
    public:

        TestMethodOpenDb() : Method("open_db")
        {}

        virtual void exec(
            common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            common::SharedPtr<Context> ctx,
            Request request,
            Callback callback
        ) override;
};

class HATN_API_EXPORT TestMethodDestroyDb : public Method
{
    public:

        TestMethodDestroyDb() : Method("destroy_db")
        {}

        virtual void exec(
            common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            common::SharedPtr<Context> ctx,
            Request request,
            Callback callback
        ) override;
};

class HATN_API_EXPORT TestServiceDb : public Service
{
    public:

        TestServiceDb(HATN_APP_NAMESPACE::App* app);
};

HATN_API_CLIENT_BRIDGE_NAMESPACE_END

#endif // HATNCLIENTTESSERVICEDB_H
