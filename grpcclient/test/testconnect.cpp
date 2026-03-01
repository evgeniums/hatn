/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/test/testconnect.cpp
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

#include <hatn/api/api.h>

#include <hatn/app/app.h>

#include <hatn/grpcclient/grpcrouter.h>
#include <hatn/grpcclient/grpcclient.h>

#include <hatn/api/client/session.h>
#include <hatn/api/client/serviceclient.h>

#include <hatn/clientserver/auth/clientsessionsharedsecret.h>
#include <hatn/clientserver/clientwithauth.h>
#include <hatn/clientserver/ipp/clientsession.ipp>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/clientrequest.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/session.ipp>
#include <hatn/api/ipp/makeapierror.ipp>
#include <hatn/grpcclient/ipp/grpctransport.ipp>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

using GrpcClientWithAuth=ClientWithSharedSecretAuthT<HATN_GRPCCLIENT_NAMESPACE::Router,HATN_GRPCCLIENT_NAMESPACE::GrpcClient>;

// constexpr makeClientWithSharedSecretAuthContextT<HATN_GRPCCLIENT_NAMESPACE::Router,HATN_GRPCCLIENT_NAMESPACE::GrpcClient> makeGrpcClientWithAuthContext;

HATN_CLIENT_SERVER_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_CLIENT_SERVER_NAMESPACE::GrpcClientWithAuth,HATN_CLIENT_SERVER_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_CLIENT_SERVER_NAMESPACE::GrpcClientWithAuth::Client,HATN_CLIENT_SERVER_EXPORT)

HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::GrpcClientWithAuth,GrpcClientWithAuth)
HATN_TASK_CONTEXT_DEFINE(HATN_CLIENT_SERVER_NAMESPACE::GrpcClientWithAuth::Client,GrpcClientWithAuthClient)

HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING
HATN_USING
HATN_TEST_USING

namespace clientserver=HATN_CLIENT_SERVER_NAMESPACE;

/********************** TestEnv **************************/

struct TestEnv : public ::hatn::test::MultiThreadFixture
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

constexpr const uint32_t TcpPort=5000;

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

using ClientType=grpcclient::GrpcClient<grpcclient::Router,client::SessionWrapper<client::SessionNoAuth,client::SessionNoAuthContext>>;
using ClientCtxType=client::ClientContext<ClientType>;
HATN_TASK_CONTEXT_DECLARE(ClientType)
HATN_TASK_CONTEXT_DEFINE(ClientType,ClientType)

auto createClientApp(std::string configFileName)
{
    app::AppName appName{"authclient","Grpc Client"};
    auto app=std::make_shared<app::App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("grpcclient",configFileName);
    auto ec=app->loadConfigFile(configFile);
    HATN_TEST_EC(ec);
    BOOST_REQUIRE(!ec);
    ec=app->init();
    HATN_TEST_EC(ec);
    BOOST_REQUIRE(!ec);
    return app;
}

auto createClient(std::shared_ptr<app::App> app)
{
    auto thread=app->appThread();
    const pmr::AllocatorFactory* factory=app->allocatorFactory().factory();
    auto router=makeShared<grpcclient::Router>();
    router->setHost("localhost",TcpPort);
    router->setInsecure(true);
    auto ctx=client::makeClientContext<client::ClientContext<ClientType>>(        
        thread,
        factory
    );

    auto& logContext=ctx->get<HATN_LOGCONTEXT_NAMESPACE::Context>();
    logContext.setLogger(app->logger().logger());

    HATN_BASE_NAMESPACE::config_object::LogRecords records;
    auto& client=ctx->get<ClientType>();
    std::ignore=client.loadLogConfig(app->configTree(),"grpc",records,{});

    client.setRouter(std::move(router));

    return ctx;
}

#if 0
auto createClient(ThreadQWithTaskContext* thread)
{
    auto resolver=std::make_shared<client::IpHostResolver>(thread);
#if 0
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
#else
    auto router=makeShared<grpcclient::Router>();
#endif
    router->setHost("localhost",TcpPort);
    router->setInsecure(true);
    auto cl=client::makeClientContext<client::ClientContext<ClientType>>(
                std::move(router),
                thread
            );

    const HATN_BASE_NAMESPACE::ConfigTree configTree;
    HATN_BASE_NAMESPACE::config_object::LogRecords records;
    auto& client=cl->get<ClientType>();
    std::ignore=client.loadLogConfig(configTree,{},records,{});
    return cl;
}
#endif

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

/********************** Tests **************************/

BOOST_AUTO_TEST_SUITE(TestConnect)

#if 0
BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    createThreads(2);
    auto workThread=threadWithContextTask(1);

    std::ignore=createClient(workThread.get());

    auto grpcClientCtx=createGrpcClient();
    auto& session=grpcClientCtx->get<clientserver::GrpcClientWithAuth::Session>();
    session.setSerializedHeaderNeeded(false);

    BOOST_CHECK(true);
}
#endif

BOOST_FIXTURE_TEST_CASE(Echo,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("echo.jsonc");
    auto client=createClient(app);
    auto session=client::makeSessionNoAuthContext();
    auto clientWithAuth=createClientWithAuth(client,session);
    session.setSerializedHeaderNeeded(false);

    auto serviceClient=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("Status",clientWithAuth);
    serviceClient->setPackage("generic_api");

    clientThread->start();

    api::Method echoMethod{"Echo"};
    auto invokeEcho=[serviceClient,app,&echoMethod]()
    {
        auto cb=[](auto ctx, const Error& ec, auto response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeTask1 cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_CHECK(!ec);
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());
        service1_msg1::type msg;
        msg.setFieldValue(service1_msg1::field1,100);
        msg.setFieldValue(service1_msg1::field2,"hello world!");
        serviceClient->exec(
            ctx,
            cb,
            echoMethod,
            msg,
            "topic1"
        );
    };

    clientThread->execAsync(invokeEcho);

    int secs=180;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

#if 0
BOOST_FIXTURE_TEST_CASE(TestExec,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto session=client::makeSessionNoAuthContext();
    auto client=createClient(clientThread.get());
    auto clientWithAuth=createClientWithAuth(client,session);
    session.setSerializedHeaderNeeded(false);

    auto session1=clientserver::makeSessionSharedSecretContext();
    session1.setSerializedHeaderNeeded(false);

    auto service1Client=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("service1",clientWithAuth);
    service1Client->setPackage("example1");
    auto service2Client=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("service2",clientWithAuth);
    service2Client->setPackage("example1");

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
        service1Client->exec(
            ctx,
            cb,
            "service1_method1",
            msg,
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

    BOOST_CHECK(true);
}
#endif
BOOST_AUTO_TEST_SUITE_END()

