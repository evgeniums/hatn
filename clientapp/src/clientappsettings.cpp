/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientappsettings.сpp
  *
  */

#include <hatn/base/configtreejson.h>

#include <hatn/logcontext/postasync.h>

#include <hatn/db/asyncclient.h>
#include <hatn/db/client.h>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientappdbmodels.h>
#include <hatn/clientapp/clientappsettings.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

ClientAppSettings::ClientAppSettings(ClientApp* app) : m_app(app)
{}

//---------------------------------------------------------------

Error ClientAppSettings::load()
{
    m_configTree.reset();

    using dbResult=Result<HATN_DB_NAMESPACE::DbObjectT<clientapp_data::managed>>;

    auto future=m_app->app().appThread()->execFuture<dbResult>(
        [self=shared_from_this(),this]()
        {
            auto q=HATN_DB_NAMESPACE::makeQuery(clientAppDataIdx(),
                                                  db::where(clientapp_data::name,db::query::eq,DbObjectName),
                                                  ClientAppDbModels::DefaultTopic
                                                  );
            return m_app->mainDb().dbClient()->client()->findOne(
                    ClientAppDbModels::defaultInstance()->clientAppDataModel(),
                    q
                );
        }
    );
    future.wait();
    auto r=future.get();
    if (r)
    {
        if (r.error().is(HATN_DB_NAMESPACE::DbError::NOT_FOUND))
        {
            return OK;
        }
        return r.takeError();
    }

    HATN_BASE_NAMESPACE::ConfigTreeJson parser;
    auto ec=parser.parse(m_configTree,r.value().as<clientapp_data::managed>()->fieldValue(clientapp_data::data));
    if (ec)
    {
        m_configTree.reset();
        return ec;
    }
    return OK;
}

//---------------------------------------------------------------

void ClientAppSettings::flush(
        common::SharedPtr<Context> ctx,
        Callback callback
    )
{
    postAsync(
        "appsettings::flush",
        common::ThreadQWithTaskContext::current(),
        std::move(ctx),
        [self=shared_from_this(),this](auto ctx, auto callback)
        {
            HATN_BASE_NAMESPACE::ConfigTreeJson serializer;
            auto r=serializer.serialize(m_configTree);
            if (r)
            {
                callback(std::move(ctx),r.error());
                return;
            }

            auto cb=[callback=std::move(callback)](auto ctx, auto result) mutable
            {
                if (result)
                {
                    callback(std::move(ctx),result.error());
                    return;
                }
                callback(std::move(ctx),Error{});
            };

            auto q=HATN_DB_NAMESPACE::wrapQueryBuilder(
                []()
                {
                    return HATN_DB_NAMESPACE::makeQuery(clientAppDataIdx(),
                                         db::where(clientapp_data::name,db::query::eq,DbObjectName),
                                         ClientAppDbModels::DefaultTopic
                                         );
                },
                ClientAppDbModels::DefaultTopic
            );

            auto dbObj=m_app->app().allocatorFactory().factory()->createObject<clientapp_data::managed>();
            db::initObject(*dbObj);
            dbObj->setFieldValue(clientapp_data::name,DbObjectName);
            dbObj->field(clientapp_data::data).buf(true)->load(r.value());

            auto updateReq=HATN_DB_NAMESPACE::update::sharedRequest(
                HATN_DB_NAMESPACE::update::field(clientapp_data::data,HATN_DB_NAMESPACE::update::set,dbObj->fieldValue(clientapp_data::data))
            );

            m_app->mainDb().dbClient()->findUpdateCreate(
                std::move(ctx),
                std::move(cb),
                ClientAppDbModels::defaultInstance()->clientAppDataModel(),
                std::move(q),
                std::move(updateReq),
                std::move(dbObj),
                HATN_DB_NAMESPACE::update::ModifyReturn::After,
                nullptr,
                ClientAppDbModels::DefaultTopic
            );
        },
        std::move(callback)
    );
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
