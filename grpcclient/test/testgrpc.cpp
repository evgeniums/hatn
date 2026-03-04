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
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/dataunit/compare.h>

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
#include <hatn/clientserver/ipp/clientwithauth.ipp>
#include <hatn/clientserver/ipp/clientauthprotocolsharedsecret.ipp>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/clientrequest.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/session.ipp>
#include <hatn/api/ipp/makeapierror.ipp>
#include <hatn/grpcclient/ipp/grpctransport.ipp>

constexpr const static int TEST_DURATION=5;

using GrpcClinetCtxBuilder= HATN_CLIENT_SERVER_NAMESPACE::makeClientWithSharedSecretAuthContextT<HATN_GRPCCLIENT_NAMESPACE::Router,
                                                                                                 HATN_GRPCCLIENT_NAMESPACE::GrpcClient>;

using GrpcClientWithAuthCtx=GrpcClinetCtxBuilder::type;
using GrpcClientWithAuth=HATN_CLIENT_SERVER_NAMESPACE::ClientWithSharedSecretAuthT<HATN_GRPCCLIENT_NAMESPACE::Router,
                                                                                HATN_GRPCCLIENT_NAMESPACE::GrpcClient>;

GrpcClinetCtxBuilder makeGrpcClientWithAuthCtx;

HATN_TASK_CONTEXT_DECLARE(GrpcClientWithAuth)
HATN_TASK_CONTEXT_DECLARE(GrpcClientWithAuth::Client)

HATN_TASK_CONTEXT_DEFINE(GrpcClientWithAuth,GrpcClientWithAuth)
HATN_TASK_CONTEXT_DEFINE(GrpcClientWithAuth::Client,GrpcClientWithAuthClient)

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

constexpr const uint32_t TcpPort=25273;

/********************** Messages **************************/

HDU_UNIT(service1_msg1,
    HDU_FIELD(field1,TYPE_UINT32,1)
    HDU_FIELD(field2,TYPE_STRING,2)
)

HDU_UNIT(basic,
    HDU_FIELD(vsint32,TYPE_INT32,1)
    HDU_FIELD(vsint64,TYPE_INT64,2)
    HDU_FIELD(vuint32,TYPE_UINT32,3)
    HDU_FIELD(vuint64,TYPE_UINT64,4)
    HDU_FIELD(vsfixed32,TYPE_FIXED_INT32,5)
    HDU_FIELD(vsfixed64,TYPE_FIXED_INT64,6)
    HDU_FIELD(vfixed32,TYPE_FIXED_UINT32,7)
    HDU_FIELD(vfixed64,TYPE_FIXED_UINT64,8)
    HDU_FIELD(vfloat,TYPE_FLOAT,9)
    HDU_FIELD(vdouble,TYPE_DOUBLE,10)
    HDU_FIELD(vboolt,TYPE_BOOL,11)
    HDU_FIELD(vboolf,TYPE_BOOL,12)
    HDU_FIELD(vstring,TYPE_STRING,13)
    HDU_FIELD(vbytes,TYPE_BYTES,14)
)

HDU_UNIT(repeated,
    HDU_REPEATED_FIELD(vsint32,TYPE_INT32,1)
    HDU_REPEATED_FIELD(vsint64,TYPE_INT64,2)
    HDU_REPEATED_FIELD(vuint32,TYPE_UINT32,3)
    HDU_REPEATED_FIELD(vuint64,TYPE_UINT64,4)
    HDU_REPEATED_FIELD(vsfixed32,TYPE_FIXED_INT32,5)
    HDU_REPEATED_FIELD(vsfixed64,TYPE_FIXED_INT64,6)
    HDU_REPEATED_FIELD(vfixed32,TYPE_FIXED_UINT32,7)
    HDU_REPEATED_FIELD(vfixed64,TYPE_FIXED_UINT64,8)
    HDU_REPEATED_FIELD(vfloat,TYPE_FLOAT,9)
    HDU_REPEATED_FIELD(vdouble,TYPE_DOUBLE,10)
    HDU_REPEATED_FIELD(vboolt,TYPE_BOOL,11)
    HDU_REPEATED_FIELD(vboolf,TYPE_BOOL,12)
    HDU_REPEATED_FIELD(vstring,TYPE_STRING,13)
    HDU_REPEATED_FIELD(vbytes,TYPE_BYTES,14)
)

HDU_UNIT(embedded,
    HDU_FIELD(f1,basic::TYPE,1)
    HDU_FIELD(f2,repeated::TYPE,2)
    HDU_REPEATED_FIELD(f3,basic::TYPE,3)
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

    auto& client=ctx->get<ClientType>();
    auto ec=loadLogConfig("Configuration of grpc client",client,app->configTree(),"grpc");
    HATN_TEST_EC(ec)

    client.setRouter(std::move(router));

    return ctx;
}

auto createSessionClient(std::shared_ptr<app::App> app)
{
    auto thread=app->appThread();
    const pmr::AllocatorFactory* factory=app->allocatorFactory().factory();
    auto router=makeShared<grpcclient::Router>();
    router->setHost("localhost",TcpPort);
    router->setInsecure(true);

    auto ctx=makeGrpcClientWithAuthCtx(
        common::subcontexts(
            common::subcontext(),
            common::subcontext(
                    thread,
                    factory
                ),
            common::subcontext(),
            common::subcontext()
        )
    );
    auto& session=ctx->get<GrpcClientWithAuth::Session>();
    session.setSerializedHeaderNeeded(false);

    auto& logContext=ctx->get<HATN_LOGCONTEXT_NAMESPACE::Context>();
    logContext.setLogger(app->logger().logger());

    auto& client=ctx->get<GrpcClientWithAuth::Client>();
    auto ec=loadLogConfig("Configuration of grpc client",client,app->configTree(),"grpc");
    HATN_TEST_EC(ec)

    client.setRouter(std::move(router));

    return ctx;
}

using ClientWithAuthType=client::ClientWithAuth<
    client::SessionWrapper<
            client::SessionNoAuth,
            client::SessionNoAuthContext
        >,
    client::ClientContext<ClientType>,
    ClientType>;
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

BOOST_AUTO_TEST_SUITE(TestGrpc)

BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    BOOST_TEST_MESSAGE("create app");
    auto app=createClientApp("grpcclient.jsonc");

    BOOST_TEST_MESSAGE("create no auth client");
    auto client=createClient(app);

    BOOST_TEST_MESSAGE("create client with shared secret session");
    auto sessionClient=createSessionClient(app);

    clientThread->start();

    BOOST_TEST_MESSAGE("waiting 3 seconds...");
    exec(3);
    BOOST_TEST_MESSAGE("done");
    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(Echo,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("grpcclient.jsonc");
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
        auto msg=common::makeShared<service1_msg1::managed>();

        auto cb=[msg](auto ctx, const Error& ec, api::client::Response response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeEcho cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_REQUIRE(!ec);

            auto msg1=common::makeShared<service1_msg1::managed>();
            HATN_DATAUNIT_NAMESPACE::WireBufSolidShared buf{response.messageData()};
            auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(*msg1,buf);
            BOOST_CHECK(ok);

            HATN_TEST_MESSAGE_TS(fmt::format("Echo received: {}",msg1->toString(true)));

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(msg,msg1));
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());        
        msg->setFieldValue(service1_msg1::field1,100);
        msg->setFieldValue(service1_msg1::field2,"hello world!");
        HATN_TEST_MESSAGE_TS(fmt::format("Echo send: {}",msg->toString(true)));
        auto ec=serviceClient->exec(
            ctx,
            cb,
            echoMethod,
            *msg,
            "topic1"
        );
        HATN_TEST_EC(ec)
    };

    clientThread->execAsync(invokeEcho);

    int secs=TEST_DURATION;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(Basic,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("grpcclient.jsonc");
    auto client=createClient(app);
    auto session=client::makeSessionNoAuthContext();
    auto clientWithAuth=createClientWithAuth(client,session);
    session.setSerializedHeaderNeeded(false);
    auto serviceClient=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("GrpcTest",clientWithAuth);
    serviceClient->setPackage("grpc_api");

    clientThread->start();

    api::Method basicTypesMethod{"Basic"};
    auto invokeEcho=[serviceClient,app,&basicTypesMethod]()
    {
        auto msg=common::makeShared<basic::managed>();
        msg->setFieldValue(basic::vsint32, -1234);
        msg->setFieldValue(basic::vsint64, -1234567890);
        msg->setFieldValue(basic::vuint32, 1234);
        msg->setFieldValue(basic::vuint64, 1234567890);
        msg->setFieldValue(basic::vsfixed32, -5678);
        msg->setFieldValue(basic::vsfixed64, -5678901234);
        msg->setFieldValue(basic::vfixed32, 5678);
        msg->setFieldValue(basic::vfixed64, 5678901234);
        msg->setFieldValue(basic::vfloat, 12.34);
        msg->setFieldValue(basic::vdouble, double(1234.3456));
        msg->setFieldValue(basic::vboolt, true);
        msg->setFieldValue(basic::vboolf, false);
        msg->setFieldValue(basic::vstring, "Hello world!");
        msg->setFieldValue(basic::vbytes, "Hi, how are you?");

        auto cb=[msg](auto ctx, const Error& ec, api::client::Response response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeBasic cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_REQUIRE(!ec);

            auto msg1=common::makeShared<basic::managed>();
            HATN_DATAUNIT_NAMESPACE::WireBufSolidShared buf{response.messageData()};
            auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(*msg1,buf);
            BOOST_CHECK(ok);

            HATN_TEST_MESSAGE_TS(fmt::format("Response received: {}",msg1->toString(true)));

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(msg,msg1));
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());
        HATN_TEST_MESSAGE_TS(fmt::format("Command to send: {}",msg->toString(true)));
        auto ec=serviceClient->exec(
            ctx,
            cb,
            basicTypesMethod,
            *msg,
            "topic1"
        );
        HATN_TEST_EC(ec)
    };

    clientThread->execAsync(invokeEcho);

    int secs=TEST_DURATION;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(Repeated,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("grpcclient.jsonc");
    auto client=createClient(app);
    auto session=client::makeSessionNoAuthContext();
    auto clientWithAuth=createClientWithAuth(client,session);
    session.setSerializedHeaderNeeded(false);
    auto serviceClient=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("GrpcTest",clientWithAuth);
    serviceClient->setPackage("grpc_api");

    clientThread->start();

    api::Method basicTypesMethod{"Repeated"};
    auto invokeEcho=[serviceClient,app,&basicTypesMethod]()
    {
        auto msg=common::makeShared<repeated::managed>();
        msg->field(repeated::vsint32).append(-1234);
        msg->field(repeated::vsint64).append( -1234567890);
        msg->field(repeated::vuint32).append( 1234);
        msg->field(repeated::vuint64).append( 1234567890);
        msg->field(repeated::vsfixed32).append( -5678);
        msg->field(repeated::vsfixed64).append( -5678901234);
        msg->field(repeated::vfixed32).append( 5678);
        msg->field(repeated::vfixed64).append( 5678901234);
        msg->field(repeated::vfloat).append( 12.34);
        msg->field(repeated::vdouble).append( double(1234.3456));
        msg->field(repeated::vboolt).append( true);
        msg->field(repeated::vboolf).append( false);
        msg->field(repeated::vstring).append( "Hello world!");
        msg->field(repeated::vbytes).append( "Hi, how are you?");

        auto cb=[msg](auto ctx, const Error& ec, api::client::Response response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeRepeated cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_REQUIRE(!ec);

            auto msg1=common::makeShared<repeated::managed>();
            HATN_DATAUNIT_NAMESPACE::WireBufSolidShared buf{response.messageData()};
            auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(*msg1,buf);
            BOOST_CHECK(ok);

            HATN_TEST_MESSAGE_TS(fmt::format("Response received: {}",msg1->toString(true)));

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsint32,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsint64,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vuint32,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vuint64,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsfixed32,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsfixed64,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfixed32,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfixed64,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfloat,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vdouble,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vboolt,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vboolf,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vstring,msg,msg1));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vbytes,msg,msg1));
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());
        HATN_TEST_MESSAGE_TS(fmt::format("Command to send: {}",msg->toString(true)));
        auto ec=serviceClient->exec(
            ctx,
            cb,
            basicTypesMethod,
            *msg,
            "topic1"
        );
        HATN_TEST_EC(ec)
    };

    clientThread->execAsync(invokeEcho);

    int secs=TEST_DURATION;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(Embedded,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("grpcclient.jsonc");
    auto client=createClient(app);
    auto session=client::makeSessionNoAuthContext();
    auto clientWithAuth=createClientWithAuth(client,session);
    session.setSerializedHeaderNeeded(false);
    auto serviceClient=makeShared<client::ServiceClient<ClientWithAuthCtxType,ClientWithAuthType>>("GrpcTest",clientWithAuth);
    serviceClient->setPackage("grpc_api");

    clientThread->start();

    api::Method basicTypesMethod{"Embedded"};
    auto invokeEcho=[serviceClient,app,&basicTypesMethod]()
    {
        auto msg=common::makeShared<embedded::managed>();
        auto f1=msg->mutableMember(embedded::f1)->createShared();
        f1->setFieldValue(basic::vsint32, -1234);
        f1->setFieldValue(basic::vsint64, -1234567890);
        f1->setFieldValue(basic::vuint32, 1234);
        f1->setFieldValue(basic::vuint64, 1234567890);
        f1->setFieldValue(basic::vsfixed32, -5678);
        f1->setFieldValue(basic::vsfixed64, -5678901234);
        f1->setFieldValue(basic::vfixed32, 5678);
        f1->setFieldValue(basic::vfixed64, 5678901234);
        f1->setFieldValue(basic::vfloat, 12.34);
        f1->setFieldValue(basic::vdouble, double(1234.3456));
        f1->setFieldValue(basic::vboolt, true);
        f1->setFieldValue(basic::vboolf, false);
        f1->setFieldValue(basic::vstring, "Hello world!");
        f1->setFieldValue(basic::vbytes, "Hi, how are you?");
        auto f2=msg->mutableMember(embedded::f2)->createShared();
        f2->field(repeated::vsint32).append(-1234);
        f2->field(repeated::vsint64).append( -1234567890);
        f2->field(repeated::vuint32).append( 1234);
        f2->field(repeated::vuint64).append( 1234567890);
        f2->field(repeated::vsfixed32).append( -5678);
        f2->field(repeated::vsfixed64).append( -5678901234);
        f2->field(repeated::vfixed32).append( 5678);
        f2->field(repeated::vfixed64).append( 5678901234);
        f2->field(repeated::vfloat).append( 12.34);
        f2->field(repeated::vdouble).append( double(1234.3456));
        f2->field(repeated::vboolt).append( true);
        f2->field(repeated::vboolf).append( false);
        f2->field(repeated::vstring).append( "Hello world!");
        f2->field(repeated::vbytes).append( "Hi, how are you?");

        auto f3=msg->mutableMember(embedded::f3)->appendSharedSubunit().sharedValue();
        f3->setFieldValue(basic::vsint32, -1234);
        f3->setFieldValue(basic::vsint64, -1234567890);
        f3->setFieldValue(basic::vuint32, 1234);
        f3->setFieldValue(basic::vuint64, 1234567890);
        f3->setFieldValue(basic::vsfixed32, -5678);
        f3->setFieldValue(basic::vsfixed64, -5678901234);
        f3->setFieldValue(basic::vfixed32, 5678);
        f3->setFieldValue(basic::vfixed64, 5678901234);
        f3->setFieldValue(basic::vfloat, 12.34);
        f3->setFieldValue(basic::vdouble, double(1234.3456));
        f3->setFieldValue(basic::vboolt, true);
        f3->setFieldValue(basic::vboolf, false);
        f3->setFieldValue(basic::vstring, "Hello world!");
        f3->setFieldValue(basic::vbytes, "Hi, how are you?");

        auto cb=[msg,f1,f2](auto ctx, const Error& ec, api::client::Response response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeEmbedded cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_REQUIRE(!ec);

            auto msg1=common::makeShared<embedded::managed>();
            HATN_DATAUNIT_NAMESPACE::WireBufSolidShared buf{response.messageData()};
            auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(*msg1,buf);
            BOOST_CHECK(ok);

            HATN_TEST_MESSAGE_TS(fmt::format("Response received: {}",msg1->toString(true)));

            auto f11=msg1->field(embedded::f1).sharedValue();
            auto f12=msg1->field(embedded::f2).sharedValue();

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsint32,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsint64,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vuint32,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vuint64,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsfixed32,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vsfixed64,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfixed32,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfixed64,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vfloat,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vdouble,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vboolt,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vboolf,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vstring,f2,f12));
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::repeatedFieldsEqual(repeated::vbytes,f2,f12));

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(f1,f11));

            const auto& ff3=msg1->field(embedded::f3);
            BOOST_REQUIRE_EQUAL(ff3.count(),1);
            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(f1,ff3.at(0).sharedValue()));
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());
        HATN_TEST_MESSAGE_TS(fmt::format("Command to send: {}",msg->toString(true)));
        auto ec=serviceClient->exec(
            ctx,
            cb,
            basicTypesMethod,
            *msg,
            "topic1"
        );
        HATN_TEST_EC(ec)
    };

    clientThread->execAsync(invokeEcho);

    int secs=TEST_DURATION;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(AuthTokenBearer,TestEnv)
{
    createThreads(1);
    auto clientThread=threadWithContextTask(0);

    auto app=createClientApp("grpcclient.jsonc");
    HATN_CTX_ENTER_SCOPE("AuthTokenBearer")
    BOOST_TEST_MESSAGE("create client with shared secret session");
    auto client=createSessionClient(app);
    auto& session=client->get<GrpcClientWithAuth::Session>();

    std::string tokenString="kwsjdlhfqiwue;fiquh";
    HATN_CLIENT_SERVER_NAMESPACE::auth_token::type token;
    token.setFieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::token,tokenString);
    HATN_DATAUNIT_NAMESPACE::WireBufSolid wbuf;
    BOOST_CHECK_GT(HATN_DATAUNIT_NAMESPACE::io::serialize(token,wbuf),0);
    auto ec=session.sessionImpl().loadSessionToken(wbuf.mainContainer()->stringView());
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    auto serviceClient=makeShared<client::ServiceClient<GrpcClientWithAuthCtx,GrpcClientWithAuth>>("GrpcTest",client);
    serviceClient->setPackage("grpc_api");

    clientThread->start();

    api::Method echoMethod{"EchoToken"};
    auto invokeEcho=[serviceClient,app,&echoMethod]()
    {
        auto msg=common::makeShared<service1_msg1::managed>();

        auto cb=[msg](auto ctx, const Error& ec, api::client::Response response)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("invokeEcho cb, ec: {}/{}",ec.value(),ec.message()));
            BOOST_REQUIRE(!ec);

            auto msg1=common::makeShared<service1_msg1::managed>();
            HATN_DATAUNIT_NAMESPACE::WireBufSolidShared buf{response.messageData()};
            auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(*msg1,buf);
            BOOST_CHECK(ok);

            HATN_TEST_MESSAGE_TS(fmt::format("Echo received: {}",msg1->toString(true)));

            BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(msg,msg1));
        };

        auto ctx=makeLogCtx();
        auto& logCtx=ctx->get<LogContext>();
        logCtx.setLogger(app->logger().logger());
        msg->setFieldValue(service1_msg1::field1,100);
        msg->setFieldValue(service1_msg1::field2,"hello world!");
        HATN_TEST_MESSAGE_TS(fmt::format("Echo send: {}",msg->toString(true)));
        auto ec=serviceClient->exec(
            ctx,
            cb,
            echoMethod,
            *msg,
            "topic1"
        );
        HATN_TEST_EC(ec)
    };

    clientThread->execAsync(invokeEcho);

    int secs=180;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    clientThread->stop();

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

