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

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/base/configobject.h>
#include <hatn/base/configtreejson.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientbridge.h>
#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/mobileplatformcontext.h>
#include <hatn/clientapp/testservicedb.h>
#include <hatn/clientapp/mobileapp.h>

#include <hatn/common/logger.h>
#include <hatn/common/loggermoduleimp.h>

HATN_LOG_MODULE_DECLARE_EXP(mobileapp,HATN_CLIENTAPP_EXPORT)
HATN_LOG_MODULE_INIT(mobileapp,HATN_CLIENTAPP_EXPORT)

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

constexpr const char* TestingSection="testing";

HDU_UNIT(testing_config,
    HDU_FIELD(enable,TYPE_UINT8,1)
)

using TestingConfig=HATN_BASE_NAMESPACE::ConfigObject<testing_config::type>;

//-----------------------------------------------------------------------------

class MobileApp_p
{
    public:

        MobileApp_p(HATN_APP_NAMESPACE::AppName appName) : app(std::move(appName))
        {}

        HATN_CLIENTAPP_NAMESPACE::ClientApp app;
        MobilePlatformContext* platformCtx=nullptr;
};

//-----------------------------------------------------------------------------

MobileApp::MobileApp(HATN_APP_NAMESPACE::AppName appName) : pimpl(std::make_unique<MobileApp_p>(std::move(appName)))
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
    pimpl->app.app().setAppDataFolder(std::move(dataDir));

    // load configuration file
    auto ec=pimpl->app.app().loadConfigFile(configFile);
    if (ec)
    {
        return -1;
    }

    // init platform context
    ec=platformCtx->init(this);
    if (ec)
    {
        return -2;
    }
    pimpl->platformCtx=platformCtx;

    // init app
    ec=pimpl->app.app().init();
    if (ec)
    {
        close();
        return -3;
    }

    // set default env of client bridge
    pimpl->app.bridge().setDefaultEnv(pimpl->app.app().env());

    // log info
    HATN_CTX_INFO_RECORDS_M("RUNNING MOBILE APP",HLOG_MODULE(mobileapp),{"name",pimpl->app.app().appName().displayName})

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
    pimpl->app.app().close();

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
    HATN_CLIENTAPP_NAMESPACE::Request req{
        std::move(request.envId),
        std::move(request.topic),
        std::move(request.messageTypeName)
    };
    if (req.messageTypeName!="")
    {
        auto msgR=pimpl->app.bridge().makeMessage(service,req.messageTypeName,request.messageJson);
        if (msgR)
        {
            HATN_CTX_ERROR_RECORDS_M(msgR.error(),HLOG_MODULE(mobileapp),"failed to make message",{"service",service},{"method",method})
            callback(Error{msgR.error().value(),msgR.error().codeString(),msgR.error().message()},Response{});
            return;
        }
        req.message=msgR.takeValue();
    }

    auto cb=[callback,method,service](const HATN_NAMESPACE::Error& ec, HATN_CLIENTAPP_NAMESPACE::Response resp)
    {
        HATN_CTX_SCOPE("execcb")
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
    pimpl->app.bridge().exec(service,method,std::move(req),std::move(cb));
}

//-----------------------------------------------------------------------------

int MobileApp::initTests()
{
    HATN_CTX_SCOPE("inittests")

    TestingConfig testingConfig;

    auto ec=HATN_NAMESPACE::loadLogConfig("configuration of tests",HLOG_MODULE(mobileapp),testingConfig,pimpl->app.app().configTree(),TestingSection);
    if (ec)
    {
        HATN_CTX_ERROR(ec,"failed to load configuration of testing",HLOG_MODULE(mobileapp))
        return 4;
    }

    if (!testingConfig.config().fieldValue(testing_config::enable))
    {
        return 0;
    }

    pimpl->app.bridge().registerService(
        std::make_shared<HATN_CLIENTAPP_NAMESPACE::TestServiceDb>(&pimpl->app.app())
    );

    return 0;
}

//-----------------------------------------------------------------------------

std::vector<std::string> MobileApp::listLogFiles() const
{
    return pimpl->app.app().listLogFiles();
}

//-----------------------------------------------------------------------------

HATN_CLIENTAPP_MOBILE_NAMESPACE_END
