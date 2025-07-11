/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/testservicedb.сpp
  *
  */

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/testservicedb.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

HDU_UNIT(open_db,
    HDU_FIELD(autocreate,TYPE_BOOL,1,false,true)
)

//--------------------------------------------------------------------------

void TestMethodOpenDb::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> /*env*/,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    auto baseAppCtx=ctx.dynamicCast<BridgeAppContext>();
    Assert(baseAppCtx,"invalid context");

    bool autoCreate=false;
    if (request.message)
    {
        autoCreate=request.message.managedUnit<open_db::managed>()->fieldValue(open_db::autocreate);
    }

    auto& withApp=baseAppCtx->get<WithClientApp>();
    auto ec=withApp.clientApp()->app().openDb(autoCreate);

    callback(std::move(ec),std::move(request));
}

//--------------------------------------------------------------------------

void TestMethodDestroyDb::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> /*env*/,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    auto baseAppCtx=ctx.dynamicCast<BridgeAppContext>();
    Assert(baseAppCtx,"invalid context");

    auto& withApp=baseAppCtx->get<WithClientApp>();
    auto ec=withApp.clientApp()->app().destroyDb();
    callback(std::move(ec),std::move(request));
}

//--------------------------------------------------------------------------

TestServiceDb::TestServiceDb(ClientApp* app) : Service("test_db")
{
    registerMethod(std::make_shared<TestMethodOpenDb>());
    registerMethod(std::make_shared<TestMethodDestroyDb>());

    setContextBuilder(std::make_shared<BridgeAppContextBuilder>(app));

    registerMessageBuilder(
        "open_db",
        [](const std::string& messageJson) -> common::Result<HATN_DATAUNIT_NAMESPACE::UnitWrapper>
        {
            auto obj=common::makeShared<open_db::managed>();
            auto ok=obj->loadFromJSON(messageJson);
            if (!ok)
            {
                return clientAppError(ClientAppError::FAILED_PARSE_BRIDGE_JSON);
            }
            return obj;
        }
    );
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
