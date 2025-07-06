/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/test/testmicroservice.cpp
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

HATN_TEST_USING

HATN_APP_USING
HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING

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

/********************** Messages **************************/

HDU_UNIT(service1_msg1,
    HDU_FIELD(field1,TYPE_UINT32,1)
    HDU_FIELD(field2,TYPE_STRING,2)
)

HDU_UNIT(service2_msg1,
    HDU_FIELD(field2,TYPE_UINT32,1)
    HDU_FIELD(field1,TYPE_STRING,2)
)

HDU_UNIT(service2_msg2,
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_STRING,2)
    HDU_FIELD(f3,TYPE_STRING,3)
)

/********************** Client **************************/

using ClientType=client::Client<client::PlainTcpRouter,client::SessionWrapper<client::SessionNoAuth,client::SessionNoAuthContext>,LogCtxType>;
using ClientCtxType=client::ClientContext<ClientType>;
HATN_TASK_CONTEXT_DECLARE(ClientType)
HATN_TASK_CONTEXT_DEFINE(ClientType,ClientType)

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

    auto cl=client::makeClientContext<client::ClientContext<ClientType>>(
                std::move(router),
                thread
            );
    return cl;
}

using ClientWithAuthType=client::ClientWithAuth<client::SessionWrapper<client::SessionNoAuth,client::SessionNoAuthContext>,client::ClientContext<ClientType>,ClientType>;
using ClientWithAuthCtxType=client::ClientWithAuthContext<ClientWithAuthType>;

HATN_TASK_CONTEXT_DECLARE(ClientWithAuthType)
HATN_TASK_CONTEXT_DEFINE(ClientWithAuthType)

template <typename SessionWrapperT>
auto createClientWithAuth(SharedPtr<ClientCtxType> clientCtx, SessionWrapperT session)
{
    auto scl=client::makeClientWithAuthContext<ClientWithAuthCtxType>(std::move(session),std::move(clientCtx));
    return scl;
}

/********************** Server **************************/

class Service1Method1Traits : public server::NoValidatorTraits
{
    public:

        using Request=server::Request<>;
        using Message=service1_msg1::managed;

        void exec(
                SharedPtr<server::RequestContext<server::Request<>>> request,
                server::RouteCb<server::Request<>> callback,
                SharedPtr<service1_msg1::managed> msg
            ) const
        {
            BOOST_TEST_MESSAGE(fmt::format("Service1 method1 exec: field1={}, field2={}",msg->fieldValue(service1_msg1::field1),msg->fieldValue(service1_msg1::field2)));

            auto& req=request->get<server::Request<>>();
            req.response.setStatus();
            callback(std::move(request));
        }
};
class Service1Method1 : public server::ServiceMethodNV<Service1Method1Traits>
{
    public:

        using Base=server::ServiceMethodNV<Service1Method1Traits>;

        Service1Method1() : Base("service1_method1")
        {}
};
using Service1=server::ServerServiceV<server::ServiceSingleMethod<Service1Method1>>;

//---------------------------------------------------------------

class Service2Method1Traits : public server::NoValidatorTraits
{
    public:

        using Request=server::Request<>;
        using Message=service2_msg1::managed;

        void exec(
            SharedPtr<server::RequestContext<server::Request<>>> request,
            server::RouteCb<server::Request<>> callback,
            SharedPtr<service2_msg1::managed> msg
            ) const
        {
            BOOST_TEST_MESSAGE(fmt::format("Service2 method1 exec: field1={}, field2={}",msg->fieldValue(service2_msg1::field1),msg->fieldValue(service2_msg1::field2)));

            auto& req=request->get<server::Request<>>();
            req.response.setStatus();
            callback(std::move(request));
        }
};
class Service2Method1 : public server::ServiceMethodV<server::ServiceMethodT<Service2Method1Traits>>
{
    public:

        using Base=server::ServiceMethodV<server::ServiceMethodT<Service2Method1Traits>>;

        Service2Method1() : Base("service2_method1")
        {}
};

class Service2Method2Traits : public server::NoValidatorTraits
{
    public:

        using Request=server::Request<>;
        using Message=service2_msg2::managed;

        void exec(
            SharedPtr<server::RequestContext<server::Request<>>> request,
            server::RouteCb<server::Request<>> callback,
            SharedPtr<service2_msg2::managed> msg
            ) const
        {
            BOOST_TEST_MESSAGE(fmt::format("Service2 method2 exec: f1={}, f2={}, f3={}",msg->fieldValue(service2_msg2::f1),msg->fieldValue(service2_msg2::f2),msg->fieldValue(service2_msg2::f3)));

            auto& req=request->get<server::Request<>>();
            req.response.setStatus();
            callback(std::move(request));
        }
};
class Service2Method2 : public server::ServiceMethodV<server::ServiceMethodT<Service2Method2Traits>>
{
    public:

        using Base=server::ServiceMethodV<server::ServiceMethodT<Service2Method2Traits>>;

        Service2Method2() : Base("service2_method2")
        {}
};

using Service2=server::ServerServiceV<server::ServiceMultipleMethods<>>;

//---------------------------------------------------------------

struct ServerApp
{
    std::shared_ptr<App> app;
    std::map<std::string,std::shared_ptr<server::MicroService>> microservices;
};

Result<ServerApp> createServer(std::string configFileName, int expectedErrorCode=0)
{
    std::string expectedFail;
    if (expectedErrorCode!=0)
    {
        expectedFail="expected: ";
    }

    // init server app
    AppName appName{"testmicroservice","Test Microservice"};
    auto app=std::make_shared<App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("api",configFileName);
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
    auto serviceRouter=std::make_shared<server::ServiceRouter<>>();
    auto service1Method1=std::make_shared<Service1Method1>();
    std::ignore=service1Method1;
    auto service1=std::make_shared<Service1>("service1");
    serviceRouter->registerLocalService(std::move(service1));
    server::ServiceMultipleMethods<> serv2;
    auto service2Method1=std::make_shared<Service2Method1>();
    auto service2Method2=std::make_shared<Service2Method2>();
    serv2.registerMethods({service2Method1,service2Method2});
    auto service2=std::make_shared<Service2>("service2",std::move(serv2));
    serviceRouter->registerLocalService(std::move(service2));
    using dispatcherType=server::ServiceDispatcher<>;
    auto dispatcher=std::make_shared<dispatcherType>(serviceRouter);

    // prepare factory
    auto dispatchers=std::make_shared<server::DispatchersStore<dispatcherType>>();
    dispatchers->registerDispatcher("simple_dispatcher1",std::move(dispatcher));
    server::PlainTcpMicroServiceBuilder microserviceBuilder{dispatchers};
    server::MicroServiceFactory factory;
    factory.registerBuilder("microservice1",microserviceBuilder);
    factory.registerBuilder("microservice2",microserviceBuilder);

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
    BOOST_REQUIRE_EQUAL(microservices.size(),2);
    BOOST_CHECK_EQUAL(microservices.begin()->first,"microservice1");
    BOOST_CHECK_EQUAL((++(microservices.begin()))->first,"microservice2");

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

BOOST_AUTO_TEST_SUITE(TestMicroService)

BOOST_FIXTURE_TEST_CASE(CreateServerOk,TestEnv)
{
    runCreateServer("microservices.jsonc",this,0);
}

BOOST_FIXTURE_TEST_CASE(MicroserviceDuplicate,TestEnv)
{
    runCreateServer("microservice-duplicate.jsonc",this,ApiLibError::DUPLICATE_MICROSERVICE);
}

BOOST_FIXTURE_TEST_CASE(MicroserviceMissingIpAddr,TestEnv)
{
    runCreateServer("microservice-missing-ip-addr.jsonc",this,ApiLibError::MICROSERVICE_RUN_FAILED);
}

BOOST_FIXTURE_TEST_CASE(MicroserviceInvalidIpAddr,TestEnv)
{
    runCreateServer("microservice-invalid-ip-addr.jsonc",this,ApiLibError::MICROSERVICE_RUN_FAILED);
}

BOOST_FIXTURE_TEST_CASE(MicroserviceUnknownDispatcher,TestEnv)
{
    runCreateServer("microservice-unknown-dispatcher.jsonc",this,ApiLibError::MICROSERVICE_CREATE_FAILED);
}

BOOST_FIXTURE_TEST_CASE(MicroserviceUnknownAuthDispatcher,TestEnv)
{
    runCreateServer("microservice-unknown-authdispatcher.jsonc",this,ApiLibError::MICROSERVICE_CREATE_FAILED);
}

BOOST_FIXTURE_TEST_CASE(MicroservicePortBusy,TestEnv)
{
    runCreateServer("microservice-port-busy.jsonc",this,ApiLibError::MICROSERVICE_RUN_FAILED);
}

BOOST_FIXTURE_TEST_CASE(TestExec,TestEnv)
{
    auto serverCtx=createServer("microservices.jsonc");

    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto session=client::makeSessionNoAuthContext();
    auto client=createClient(clientThread.get());
    auto clientWithAuth=createClientWithAuth(client,session);

    auto service1Client=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("service1",clientWithAuth);
    auto service2Client=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("service2",clientWithAuth);

    clientThread->start();

    auto invokeTask1=[service1Client]()
    {
        auto cb=[](auto ctx, const Error& ec, auto response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeTask1 cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_CHECK(!ec);
        };

        auto ctx=makeLogCtx();
        service1_msg1::type msg;
        msg.setFieldValue(service1_msg1::field1,100);
        msg.setFieldValue(service1_msg1::field2,"hello world!");
        Message msgData;
        auto ec=msgData.setContent(msg);
        BOOST_CHECK(!ec);
        service1Client->exec(
            ctx,
            cb,
            "service1_method1",
            std::move(msgData),
            "topic1"
        );
    };
    auto invokeTasks=[service2Client,invokeTask1]()
    {
        invokeTask1();

        auto cb2=[](auto ctx, const Error& ec, auto response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeTasks cb2, ec: {}/{}",ec.value(),ec.message()));
            BOOST_CHECK(!ec);
        };
        auto cb3=[](auto ctx, const Error& ec, auto response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeTasks cb3, ec: {}/{}",ec.value(),ec.message()));
            BOOST_CHECK(!ec);
        };

        auto ctx2=makeLogCtx();
        service2_msg1::type msg2;
        msg2.setFieldValue(service2_msg1::field2,200);
        msg2.setFieldValue(service2_msg1::field1,"Hi!");
        service2Client->exec(
            ctx2,
            cb2,
            "service2_method1",
            msg2,
            "topic1"
            );

        auto ctx3=makeLogCtx();
        service2_msg2::type msg3;
        msg3.setFieldValue(service2_msg2::f1,300);
        msg3.setFieldValue(service2_msg2::f2,"It is service2_msg2::f2");
        msg3.setFieldValue(service2_msg2::f3,"It is service2_msg2::f3");
        service2Client->exec(
            ctx3,
            cb3,
            "service2_method2",
            msg3,
            "topic1"
            );
    };

    clientThread->execAsync(invokeTasks);

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

BOOST_AUTO_TEST_SUITE_END()

