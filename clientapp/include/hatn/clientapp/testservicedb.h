/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/testservicedb.h
  */

/****************************************************************************/

#ifndef HATNCLIENTTESSERVICEDB_H
#define HATNCLIENTTESSERVICEDB_H

#include <hatn/api/api.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/bridgeappcontext.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class HATN_CLIENTAPP_EXPORT TestMethodOpenDb : public Method
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

class HATN_CLIENTAPP_EXPORT TestMethodDestroyDb : public Method
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

class HATN_CLIENTAPP_EXPORT TestServiceDb : public Service
{
    public:

        TestServiceDb(HATN_APP_NAMESPACE::App* app);
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTTESSERVICEDB_H
