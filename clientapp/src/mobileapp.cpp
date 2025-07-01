/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/mobileapp.cpp
  *
  */

#include <hatn/common/thread.h>

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/base/configobject.h>
#include <hatn/base/configtreejson.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/app/appenv.h>
#include <hatn/app/app.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/mobileplatformcontext.h>
#include <hatn/clientapp/testservicedb.h>
#include <hatn/clientapp/mobileapp.h>
#include <hatn/clientapp/eventdispatcher.h>

#include <hatn/common/logger.h>
#include <hatn/common/loggermoduleimp.h>

HATN_LOG_MODULE_DECLARE_EXP(mobileapp,HATN_CLIENTAPP_EXPORT)
HATN_LOG_MODULE_INIT(mobileapp,HATN_CLIENTAPP_EXPORT)

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

constexpr const char* TestingSection="testing";

HDU_UNIT(test_event,
    HDU_FIELD(category,TYPE_STRING,1)
    HDU_FIELD(event,TYPE_STRING,2)
    HDU_FIELD(period,TYPE_UINT32,3)
    HDU_FIELD(run_once,TYPE_BOOL,4)
)

HDU_UNIT(testing_config,
    HDU_FIELD(enable,TYPE_UINT8,1)
    HDU_REPEATED_FIELD(events,test_event::TYPE,2)
)

using TestingConfig=HATN_BASE_NAMESPACE::ConfigObject<testing_config::type>;

//-----------------------------------------------------------------------------

class MobileApp_p
{
    public:

        MobileApp_p(std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::ClientApp> app) : app(std::move(app))
        {}

        std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::ClientApp> app;
        MobilePlatformContext* platformCtx=nullptr;
};

//-----------------------------------------------------------------------------

MobileApp::MobileApp(std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::ClientApp> app) : pimpl(std::make_unique<MobileApp_p>(std::move(app)))
{
    HATN_COMMON_NAMESPACE::Thread::setMainThread(std::make_shared<HATN_COMMON_NAMESPACE::Thread>("main",false));
}

//-----------------------------------------------------------------------------

MobileApp::~MobileApp()
{
    HATN_COMMON_NAMESPACE::Thread::releaseMainThread();
}

//-----------------------------------------------------------------------------

int MobileApp::init(MobilePlatformContext* platformCtx, std::string configFile, std::string dataDir)
{
    if (dataDir.empty())
    {
        dataDir=std::filesystem::current_path().string();
    }

    // set application folder
    pimpl->app->app().setAppDataFolder(std::move(dataDir));

    // [reloaf app config
    auto ec=pimpl->app->preloadConfig();
    if (ec)
    {
        return -1;
    }

    // load configuration file
    ec=pimpl->app->app().loadConfigFile(configFile);
    if (ec)
    {
        return -2;
    }

    // init platform context
    ec=platformCtx->init(this);
    if (ec)
    {
        return -3;
    }
    pimpl->platformCtx=platformCtx;

    // init app
    ec=pimpl->app->init();
    if (ec)
    {
        close();
        return -4;
    }

    // set default env of client bridge
    pimpl->app->bridge().setDefaultEnv(pimpl->app->app().env());

    // init database
    ec=pimpl->app->initDb();
    if (ec)
    {
        close();
        return -5;
    }

    // init bridge services
    ec=pimpl->app->initBridgeServices();
    if (ec)
    {
        close();
        return -6;
    }

    // log info
    HATN_CTX_INFO_RECORDS_M("***** RUNNING MOBILE APP ******",HLOG_MODULE(mobileapp),{"name",pimpl->app->app().appName().displayName})

    // init tests if applicable
    auto testsRet=initTests();
    if (testsRet!=0)
    {
        close();
        return testsRet;
    }

    // done
    return 0;
}

//-----------------------------------------------------------------------------

int MobileApp::close()
{
    pimpl->app->app().close();

    if (pimpl->platformCtx)
    {
        auto ec=pimpl->platformCtx->close();
        if (ec)
        {
            return -1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------

void MobileApp::exec(
        const std::string& service,
        const std::string& method,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("mobileapp:exec")

    HATN_CLIENTAPP_NAMESPACE::Request req{
        std::move(request.envId),
        std::move(request.topic),
        std::move(request.messageTypeName)
    };

    // make message
    if (req.messageTypeName!="")
    {
        auto msgR=pimpl->app->bridge().makeMessage(service,req.messageTypeName,request.messageJson);
        if (msgR)
        {
            HATN_CTX_ERROR_RECORDS_M(msgR.error(),HLOG_MODULE(mobileapp),"failed to make message",{"bridge_srv",service},{"bridge_mthd",method})
            callback(Error{msgR.error().value(),msgR.error().codeString(),msgR.error().message()},Response{});
            return;
        }
        req.message=msgR.takeValue();
    }

    auto cb=[callback,method,service](const HATN_NAMESPACE::Error& ec, HATN_CLIENTAPP_NAMESPACE::Response resp)
    {
        HATN_CTX_SCOPE("mobileapp:exec:cb")
        if (ec)
        {
            HATN_CTX_PUSH_VAR("err",ec.codeString())
            HATN_CTX_PUSH_VAR("err_msg",ec.message())
            HATN_CTX_DEBUG(0,"failed",HLOG_MODULE(mobileapp))
        }
        else
        {
            HATN_CTX_DEBUG(0,"success",HLOG_MODULE(mobileapp))
        }
        Response response{
          std::move(resp.envId),
          std::move(resp.topic),
          std::move(resp.messageTypeName)
        };
        if (resp.message)
        {
            response.messageJson=resp.message.get()->toString();
        }
        if (!resp.buffers.empty())
        {
            response.buffers.reserve(resp.buffers.size());
            for (auto&& buffer : resp.buffers)
            {
                if (buffer)
                {
                    response.buffers.emplace_back(buffer->data(),buffer->size());
                }
            }
        }

        callback(Error{ec.value(),ec.codeString(),ec.message()},std::move(response));
    };

    // invoke
    pimpl->app->bridge().exec(service,method,std::move(req),std::move(cb));
}

//-----------------------------------------------------------------------------

size_t MobileApp::subscribeEvent(
    EventHandler handler,
    EventKey key_
)
{
    HATN_CLIENTAPP_NAMESPACE::EventKey key{
        std::move(key_.category),
        std::move(key_.event),
        std::move(key_.envId),
        std::move(key_.topic)
    };
    auto hndlr=[handler=std::move(handler)](common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
                                            common::SharedPtr<Context>,
                                            std::shared_ptr<HATN_CLIENTAPP_NAMESPACE::Event> event)
    {
        HATN_CTX_SCOPE("eventhandler")

        Event ntfcn;
        //! @todo omptimization: use referenses for similar fields instead of copying
        ntfcn.category=event->category;
        ntfcn.event=event->event;
        ntfcn.topic=event->topic;
        if (env)
        {
            ntfcn.envId=env->name();
        }
        ntfcn.messageTypeName=event->messageTypeName;
        if (event->message)
        {
            ntfcn.messageJson=event->message.get()->toString();
        }
        if (!event->buffers.empty())
        {
            ntfcn.buffers.reserve(event->buffers.size());
            for (auto&& buffer : event->buffers)
            {
                if (buffer)
                {
                    ntfcn.buffers.emplace_back(buffer->data(),buffer->size());
                }
            }
        }

        handler(ntfcn);
    };
    return pimpl->app->eventDispatcher().subscribe(std::move(hndlr),std::move(key));
}

//-----------------------------------------------------------------------------

void MobileApp::unsubscribeEvent(size_t id)
{

    return pimpl->app->eventDispatcher().unsubscribe(id);
}

//-----------------------------------------------------------------------------

int MobileApp::initTests()
{
    HATN_CTX_SCOPE("inittests")

    TestingConfig testingConfig;

    auto ec=HATN_NAMESPACE::loadLogConfig("configuration of tests",HLOG_MODULE(mobileapp),testingConfig,pimpl->app->app().configTree(),TestingSection);
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to load configuration of testing",HLOG_MODULE(mobileapp))
        return -10;
    }

    if (!testingConfig.config().fieldValue(testing_config::enable))
    {
        return 0;
    }

    pimpl->app->bridge().registerService(
        std::make_shared<HATN_CLIENTAPP_NAMESPACE::TestServiceDb>(pimpl->app.get())
    );

    const auto& events=testingConfig.config().field(testing_config::events);
    for (size_t i=0;i<events.count();i++)
    {
        std::cout << "Adding event " << i << std::endl;

        const auto& event=events.at(i);
        std::string category{event.fieldValue(test_event::category)};
        std::string name{event.fieldValue(test_event::event)};
        auto json=event.toString(true);
        auto handler=[this,category,name,json]()
        {
            DefaultContextBuilder ctxBuilder{};

            auto ctx=ctxBuilder.makeContext(pimpl->app->app().env());
            ctx->beforeThreadProcessing();

            {
                HATN_CTX_SCOPE("testevent::publish")
                HATN_CTX_INFO("publish event")

                auto event=std::make_shared<HATN_CLIENTAPP_NAMESPACE::Event>();
                event->category=category;
                event->event=name;
                event->messageTypeName=test_event::conf().name;
                auto msg=common::makeShared<test_event::managed>();
                du::WireBufSolid buf{json.data(),json.size(),true};
                msg->parse(buf);
                event->message=msg;

                pimpl->app->eventDispatcher().publish(
                    pimpl->app->app().env(),
                    ctx,
                    event
                    );
            }

            ctx->afterThreadProcessing();

            return true;
        };

        pimpl->app->app().appThread()->installTimer(
            event.fieldValue(test_event::period) * 1000 * 1000,
            handler,
            event.fieldValue(test_event::run_once)
        );
    }

    return 0;
}

//-----------------------------------------------------------------------------

std::vector<std::string> MobileApp::listLogFiles() const
{
    return pimpl->app->app().listLogFiles();
}

//-----------------------------------------------------------------------------

HATN_CLIENTAPP_MOBILE_NAMESPACE_END
