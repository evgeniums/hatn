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

constexpr makeClientWithSharedSecretAuthContextT<HATN_GRPCCLIENT_NAMESPACE::Router,HATN_GRPCCLIENT_NAMESPACE::GrpcClient> makeGrpcClientWithAuthContext;

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

struct LogCtxTraits : public client::DefaultClientTraits
{
    using Context=LogCtxType;
};

using ClientType=grpcclient::GrpcClient<grpcclient::Router,client::SessionWrapper<client::SessionNoAuth,client::SessionNoAuthContext>,LogCtxTraits>;
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
#if 0
    auto router=makeShared<client::PlainTcpRouter>(
            hosts,
            resolver,
            thread
        );
#else
    auto router=makeShared<grpcclient::Router>();
#endif
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

auto createGrpcClient()
{
    auto scl=clientserver::makeGrpcClientWithAuthContext();
    return scl;
}

/********************** Tests **************************/

BOOST_AUTO_TEST_SUITE(TestConnect)

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

#if 1
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

    BOOST_CHECK(true);
}
#endif
BOOST_AUTO_TEST_SUITE_END()

