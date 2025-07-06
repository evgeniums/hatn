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

#include <hatn/clientserver/plaintcpclientwithauth.h>

#include <hatn/serverapp/auth/authservice.h>
#include <hatn/serverapp/auth/authprotocols.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>
#include <hatn/serverapp/auth/sessionauthdispatcher.h>
#include <hatn/serverapp/logincontroller.h>
#include <hatn/serverapp/localusercontroller.h>
#include <hatn/serverapp/localsessioncontroller.h>

#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/serverresponse.ipp>
#include <hatn/api/ipp/serverservice.ipp>
#include <hatn/api/ipp/makeapierror.ipp>
#include <hatn/api/ipp/networkmicroservice.ipp>
#include <hatn/api/ipp/plaintcpmicroservice.ipp>
#include <hatn/api/ipp/serverenv.ipp>
#include <hatn/api/ipp/session.ipp>

#include <hatn/serverapp/ipp/localsessioncontroller.ipp>
#include <hatn/serverapp/ipp/sharedsecretprotocol.ipp>
#include <hatn/serverapp/ipp/logincontroller.ipp>
#include <hatn/serverapp/ipp/localusercontroller.ipp>
#include <hatn/serverapp/ipp/authservice.ipp>
#include <hatn/serverapp/ipp/sessionauthdispatcher.ipp>

#include <hatn/clientserver/ipp/clientsession.ipp>

HATN_TEST_USING

HATN_APP_USING
HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING
HATN_SERVERAPP_USING
HATN_CLIENT_SERVER_USING

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

auto createClient(ThreadQWithTaskContext* thread)
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

    const pmr::AllocatorFactory* factory=pmr::AllocatorFactory::getDefault();

    auto cl=makePlainTcpClientWithAuthContext(
        subcontexts(
            subcontext(),
            subcontext(std::move(router),thread,factory),
            subcontext(factory),
            subcontext()
        )
    );
    return cl;
}

//---------------------------------------------------------------

struct FooBar1;
struct FooBar2;

using EnvFooBar=Env<FooBar1,FooBar2>;

struct FooBar1
{
    int value()
    {
        return 0;
    }
};

struct FooBar2
{
    int value()
    {
        return env->get<FooBar1>().value();
    }

    EnvFooBar* env;
};

struct ServerApp
{
    std::shared_ptr<App> app;
    std::map<std::string,std::shared_ptr<server::MicroService>> microservices;
};

struct ContextTraits;

using EnvWithAuthT = Env<WithAuthProtocols,
                        WithSharedSecretAuthProtocol,
                        LoginController<ContextTraits>,
                        LocalUserController<ContextTraits>,
                        LocalSessionController<ContextTraits>,
                        server::BasicEnv>;

struct RequestWithAuth;

struct ContextTraits
{
    using Context=server::RequestContext<RequestWithAuth>;

    static const auto& loginController(const SharedPtr<Context>& ctx);
    static const auto& userController(const SharedPtr<Context>& ctx);
    static const auto* factory(const SharedPtr<Context>& ctx);
    static const auto& db(const SharedPtr<Context>& ctx);
};

class EnvWithAuth : public EnvWithAuthT
{
    public:

        using EnvWithAuthT::EnvWithAuthT;

        const auto& loginController() const
        {
            return get<LoginController<ContextTraits>>();
        }

        const auto& sessionController() const
        {
            return get<LocalSessionController<ContextTraits>>();
        }

        const auto& userController() const
        {
            return get<LocalUserController<ContextTraits>>();
        }
};

struct RequestWithAuth : public server::Request<EnvWithAuth>
{
    using server::Request<EnvWithAuth>::Request;

    RequestWithAuth& request()
    {
        return *this;
    }

    const RequestWithAuth& request() const
    {
        return *this;
    }
};

const auto& ContextTraits::loginController(const SharedPtr<Context>& ctx)
{
    return server::requestEnv<RequestWithAuth>(ctx)->loginController();
}

const auto& ContextTraits::userController(const SharedPtr<Context>& ctx)
{
    return server::requestEnv<RequestWithAuth>(ctx)->userController();
}

const auto* ContextTraits::factory(const SharedPtr<Context>& ctx)
{
    return server::request<RequestWithAuth>(ctx).request().factory();
}

const auto& ContextTraits::db(const SharedPtr<Context>& ctx)
{
    return server::request<RequestWithAuth>(ctx).request().db();
}

struct EnvWithAuthConfigTraits
{
    using Env=EnvWithAuth;
    using Request=RequestWithAuth;

    static Result<SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
    {
        auto sessionDbModels=std::make_shared<SessionDbModels>();

        // allocate
        auto f1=app.allocatorFactory().factory();
        auto allocator=f1->objectAllocator<Env>();
        auto env=allocateEnvType<Env>(
            allocator,
            HATN_COMMON_NAMESPACE::contexts(
                HATN_COMMON_NAMESPACE::context(std::make_shared<AuthProtocols>()),
                HATN_COMMON_NAMESPACE::context(std::make_shared<SharedSecretAuthProtocol>()),
                HATN_COMMON_NAMESPACE::context(),
                HATN_COMMON_NAMESPACE::context(),
                HATN_COMMON_NAMESPACE::context(std::move(sessionDbModels))
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

using ServiceDispatcherType=server::ServiceDispatcher<EnvWithAuth,RequestWithAuth>;
using AuthDispatcherType=SessionAuthDispatcher<EnvWithAuth,RequestWithAuth>;

using EnvWithAuthConfig=server::EnvConfigT<EnvWithAuthConfigTraits,RequestWithAuth>;
using MicroserviceBuilder=server::PlainTcpMicroServiceBuilderT<EnvWithAuthConfig,ServiceDispatcherType,AuthDispatcherType>;

HATN_TASK_CONTEXT_DECLARE(RequestWithAuth)
HATN_TASK_CONTEXT_DEFINE(RequestWithAuth,RequestWithAuth)

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
    auto serviceRouter=std::make_shared<server::ServiceRouter<EnvWithAuth,RequestWithAuth>>();
    auto authService=std::make_shared<AuthService<RequestWithAuth>>();
    serviceRouter->registerLocalService(std::move(authService));    
    auto dispatcher=std::make_shared<ServiceDispatcherType>(serviceRouter);
    auto serviceDispatchers=std::make_shared<server::DispatchersStore<ServiceDispatcherType>>();
    serviceDispatchers->registerDispatcher("default",std::move(dispatcher));

    // create suth dispatcher
    auto authDispatcher=std::make_shared<AuthDispatcherType>();
    auto authDispatchers=std::make_shared<server::DispatchersStore<AuthDispatcherType>>();
    authDispatchers->registerDispatcher("token_session",std::move(authDispatcher));

    // prepare microservice factory
    MicroserviceBuilder microserviceBuilder{serviceDispatchers,authDispatchers};
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

BOOST_FIXTURE_TEST_CASE(EnvFB,TestEnv)
{
    EnvFooBar env;
    auto& fb2=env.get<FooBar2>();
    fb2.env=&env;
    auto ret=fb2.value();
    BOOST_CHECK_EQUAL(ret,0);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,TestEnv)
{
    runCreateServer("hssauth.jsonc",this,0);
}

BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto clientCtx=createClient(clientThread.get());
    auto& cl=clientCtx->get<PlainTcpClientWithAuth>();
    std::ignore=cl.exec({},[](auto ctx, const Error& ec, client::Response){},Service{"bbb"},Method{"aaa"},{});

    clientThread->start();

    int secs=3;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);
    clientThread->stop();

    exec(1);
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

