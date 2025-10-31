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
#include <hatn/clientapp/clientbridge.h>
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
    HATN_CTX_SCOPE("clientappsettings::load")

    m_configTree.reset();

    // call synchronous db method because load() is called for initialization and blocks the app anyway
    auto q=HATN_DB_NAMESPACE::makeQuery(clientAppDataIdx(),
                                          db::where(clientapp_data::name,db::query::eq,DbObjectName),
                                          ClientAppDbModels::DefaultTopic
                                          );
    auto r=m_app->mainDb().dbClient()->client()->findOne(
            ClientAppDbModels::defaultInstance()->clientAppDataModel(),
            q
        );
    if (r)
    {
        if (r.error().is(HATN_DB_NAMESPACE::DbError::NOT_FOUND))
        {
            return OK;
        }
        return r.takeError();
    }
    else if (r.value().isNull())
    {
        return OK;
    }

    // parse config tree
    HATN_BASE_NAMESPACE::ConfigTreeJson parser;
    auto ec=parser.parse(m_configTree,r.value().as<clientapp_data::managed>()->fieldValue(clientapp_data::data));
    if (ec)
    {
        m_configTree.reset();
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void ClientAppSettings::flush(
        common::SharedPtr<Context> ctx,
        Callback callback,
        std::string section
    )
{
    if (!m_app->mainDb().dbClient())
    {
        callback(std::move(ctx),{});
        return;
    }

    m_app->bridge().execAsync(
        [ctx,callback,section,self=shared_from_this(),this]()
        {
            HATN_BASE_NAMESPACE::ConfigTreeJson serializer;
            auto r=serializer.serialize(m_configTree);
            if (r)
            {
                callback(std::move(ctx),r.error());
                return;
            }

            auto cb=[callback=std::move(callback),section=std::move(section),self,this](auto ctx, auto result) mutable
            {
                if (result)
                {
                    callback(std::move(ctx),result.error());
                    return;
                }
                callback(std::move(ctx),Error{});

                // publish event that app settings changed
                auto appSettingsEvent=std::make_shared<AppSettingsEvent>();
                appSettingsEvent->event=std::move(section);
                m_app->eventDispatcher().publish(
                    m_app->app().env(),
                    HATN_LOGCONTEXT_NAMESPACE::makeLogCtx(),
                    std::move(appSettingsEvent)
                    );
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
        }
    );
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
