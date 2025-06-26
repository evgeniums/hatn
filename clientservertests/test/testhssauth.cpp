/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientservertests/test/testhssauth.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/random.h>

#include <hatn/logcontext/streamlogger.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/app/app.h>

#include <hatn/api/api.h>

#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/client/plaintcprouter.h>
#include <hatn/api/client/client.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/serviceclient.h>

#include <hatn/api/server/plaintcpserver.h>
#include <hatn/api/server/servicerouter.h>
#include <hatn/api/server/servicedispatcher.h>
#include <hatn/api/server/connectionsstore.h>
#include <hatn/api/server/server.h>
#include <hatn/api/server/env.h>

#include <hatn/api/server/plaintcpmicroservice.h>
#include <hatn/api/server/microservicefactory.h>

#include <hatn/serverapp/auth/authservice.h>
#include <hatn/serverapp/auth/authprotocols.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/clientrequest.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/serverresponse.ipp>
#include <hatn/api/ipp/serverservice.ipp>
#include <hatn/api/ipp/session.ipp>
#include <hatn/api/ipp/makeapierror.ipp>
#include <hatn/api/ipp/networkmicroservice.ipp>
#include <hatn/api/ipp/plaintcpmicroservice.ipp>
#include <hatn/api/ipp/serverenv.ipp>

#include <hatn/serverapp/ipp/authservice.ipp>
#include <hatn/serverapp/ipp/sharedsecretprotocol.ipp>

HATN_TEST_USING

HATN_APP_USING
HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING
HATN_SERVERAPP_USING

/********************** TestEnv **************************/

struct TestEnv : public MultiThreadFixture
{
    TestEnv()
    {
    }

    ~TestEnv()
    {
    }

    TestEnv(const TestEnv&)=delete;
    TestEnv(TestEnv&&) =delete;
    TestEnv& operator=(const TestEnv&)=delete;
    TestEnv& operator=(TestEnv&&) =delete;
};

constexpr const uint32_t TcpPort=53852;

/********************** Client **************************/

using ClientType=client::Client<client::PlainTcpRouter,client::SessionWrapper<client::SessionNoAuthContext,client::SessionNoAuth>,LogCtxType>;
using ClientCtxType=client::ClientContext<ClientType>;
HATN_TASK_CONTEXT_DECLARE(ClientType)
HATN_TASK_CONTEXT_DEFINE(ClientType,ClientType)

auto createClient(Thread* thread)
{
    auto resolver=std::make_shared<client::IpHostResolver>(thread);
    std::vector<client::IpHostName> hosts;
    hosts.resize(1);
    hosts[0].name="localhost";
    hosts[0].port=TcpPort;
    hosts[0].ipVersion=IpVersion::V4;

    auto router=makeShared<client::PlainTcpRouter>(
            hosts,
            resolver,
            thread
        );

    auto cfg=std::make_shared<client::ClientConfig>();

    auto cl=client::makeClientContext<client::ClientContext<ClientType>>(
                std::move(cfg),
                std::move(router),
                thread
            );
    return cl;
}

using ClientWithAuthType=client::ClientWithAuth<client::SessionWrapper<client::SessionNoAuthContext,client::SessionNoAuth>,client::ClientContext<ClientType>,ClientType>;
using ClientWithAuthCtxType=client::ClientWithAuthContext<ClientWithAuthType>;

HATN_TASK_CONTEXT_DECLARE(ClientWithAuthType)
HATN_TASK_CONTEXT_DEFINE(ClientWithAuthType)

template <typename SessionWrapperT>
auto createClientWithAuth(SharedPtr<ClientCtxType> clientCtx, SessionWrapperT session)
{
    auto scl=client::makeClientWithAuthContext<ClientWithAuthCtxType>(std::move(session),std::move(clientCtx));
    return scl;
}

//---------------------------------------------------------------

struct ServerApp
{
    std::shared_ptr<App> app;
    std::map<std::string,std::shared_ptr<server::MicroService>> microservices;
};

using EnvWithAuth = Env<WithAuthProtocols,WithSharedSecretAuthProtocol,server::BasicEnv>;

struct EnvWithAuthConfigTraits
{
    using Env=EnvWithAuth;

    static Result<SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
    {
        // allocate
        auto f1=app.allocatorFactory().factory();
        auto allocator=f1->objectAllocator<Env>();
        auto env=allocateEnvType<Env>(
            allocator,
            HATN_COMMON_NAMESPACE::contexts(
                HATN_COMMON_NAMESPACE::context(std::make_shared<AuthProtocols>()),
                HATN_COMMON_NAMESPACE::context(std::make_shared<SharedSecretAuthProtocol>())
            ),
            server::BasicEnvConfig::prepareCtorArgs(app)
        );

        auto f2=env->get<AllocatorFactory>().factory();
        BOOST_REQUIRE_EQUAL(f1,f2);

        // init
        auto ec=server::BasicEnvConfig::initEnv(*env,configTree,configTreePath);
        HATN_CHECK_EC(ec)

        //! @todo init AuthProtocols and SharedSecretAuthProtocol

        // done
        return env;
    }
};

using EnvWithAuthConfig=server::EnvConfigT<EnvWithAuthConfigTraits>;
using MicroserviceBuilder=server::PlainTcpMicroServiceBuilderT<EnvWithAuthConfig>;

HATN_TASK_CONTEXT_DECLARE(server::Request<EnvWithAuth>)
HATN_TASK_CONTEXT_DEFINE(server::Request<EnvWithAuth>,RequestWithAuth)

HATN_TASK_CONTEXT_DECLARE(server::WithEnv<EnvWithAuth>)
HATN_TASK_CONTEXT_DEFINE(server::WithEnv<EnvWithAuth>,EnvWithAuth)

HATN_TASK_CONTEXT_DECLARE(server::PlainTcpServerT<EnvWithAuth>)
HATN_TASK_CONTEXT_DEFINE(server::PlainTcpServerT<EnvWithAuth>,TcpServer)

Result<ServerApp> createServer(std::string configFileName, int expectedErrorCode=0)
{
    std::string expectedFail;
    if (expectedErrorCode!=0)
    {
        expectedFail="expected: ";
    }

    // init server app
    AppName appName{"authserver","Auth Server"};
    auto app=std::make_shared<App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("clientservertests",configFileName);
    auto ec=app->loadConfigFile(configFile);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to load app config: {}",expectedFail,ec.message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(ec.value(),expectedErrorCode);
            return ec;
        }
    }
    BOOST_REQUIRE(!ec);
    ec=app->init();
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to init app: {}",expectedFail,ec.message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(ec.value(),expectedErrorCode);
            return ec;
        }
    }
    BOOST_REQUIRE(!ec);

    // create service dispatcher
    auto serviceRouter=std::make_shared<server::ServiceRouter<EnvWithAuth>>();
    auto authService=std::make_shared<AuthService<server::Request<EnvWithAuth>>>();
    serviceRouter->registerLocalService(std::move(authService));
    using dispatcherType=server::ServiceDispatcher<EnvWithAuth>;
    auto dispatcher=std::make_shared<dispatcherType>(serviceRouter);

    // prepare factory
    auto dispatchers=std::make_shared<server::DispatchersStore<dispatcherType>>();
    dispatchers->registerDispatcher("hss_authdispatcher",std::move(dispatcher));
    MicroserviceBuilder microserviceBuilder{dispatchers};
    server::MicroServiceFactory factory;
    factory.registerBuilder("microservice1",microserviceBuilder);

    // craete and run microservices
    auto microservicesR=factory.makeAndRunAll(*app,app->configTree());
    if (microservicesR)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to makeAndRunAll: {}",expectedFail,microservicesR.error().message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(microservicesR.error().value(),expectedErrorCode);
            app->close();
            return microservicesR.takeError();
        }
    }
    BOOST_REQUIRE(!microservicesR);

    auto microservices=microservicesR.takeValue();
    BOOST_REQUIRE_EQUAL(microservices.size(),1);
    BOOST_CHECK_EQUAL(microservices.begin()->first,"microservice1");

    return ServerApp{std::move(app),std::move(microservices)};
}

template <typename T>
void runCreateServer(std::string configFile, TestEnv* env, T expectedErrorCode)
{
    auto appCtx=createServer(configFile,static_cast<int>(expectedErrorCode));
    if (appCtx)
    {
        return;
    }

    BOOST_REQUIRE(appCtx->app);

    env->exec(3);

    for (auto&& it: appCtx->microservices)
    {
        it.second->close();
    }
    env->exec(1);
    appCtx->app->close();
}

/********************** Tests **************************/

BOOST_AUTO_TEST_SUITE(TestHssAuth)

BOOST_FIXTURE_TEST_CASE(CreateServerOk,TestEnv)
{
    runCreateServer("hssauth.jsonc",this,0);
}

#if 0
BOOST_FIXTURE_TEST_CASE(TestExec,TestEnv)
{
    auto serverCtx=createServer("microservices.jsonc");

    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto session=client::makeSessionNoAuthContext();
    auto client=createClient(clientThread.get());
    auto clientWithAuth=createClientWithAuth(client,session);

    auto service1Client=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("service1",clientWithAuth);

    clientThread->start();

    int secs=3;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    for (auto&& it: serverCtx->microservices)
    {
        it.second->close();
    }
    exec(1);
    serverCtx->app->close();

    exec(1);

    BOOST_CHECK(true);
}
#endif

BOOST_AUTO_TEST_SUITE_END()

