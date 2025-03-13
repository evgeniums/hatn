/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/test/testsendrecv.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/random.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/api/api.h>

#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/client/plaintcprouter.h>
#include <hatn/api/server/plaintcpserver.h>

#include <hatn/api/client/client.h>
#include <hatn/api/client/session.h>

#include <hatn/api/server/servicerouter.h>
#include <hatn/api/server/servicedispatcher.h>
#include <hatn/api/server/connectionsstore.h>
#include <hatn/api/server/server.h>
#include <hatn/api/server/env.h>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/request.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>

HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING

namespace {

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
}

constexpr const uint32_t TcpPort=53852;

using ClientType=client::Client<client::PlainTcpRouter,client::SessionNoAuth,LogCtxType>;
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

auto createServer(ThreadQWithTaskContext* thread, std::map<std::string,SharedPtr<server::PlainTcpConnectionContext>>& connections)
{
    auto serviceRouter=std::make_shared<server::ServiceRouter<>>();
    using dispatcherType=server::ServiceDispatcher<>;
    auto dispatcher=std::make_shared<dispatcherType>(serviceRouter);
    using connectionsStoreType=server::ConnectionsStore<server::PlainTcpConnectionContext,server::PlainTcpConnection>;
    auto connectionsStore=std::make_shared<connectionsStoreType>();

    auto server=std::make_shared<server::Server<connectionsStoreType,dispatcherType>>(std::move(connectionsStore),std::move(dispatcher));

    auto onNewTcpConnection=[server,&connections](SharedPtr<server::PlainTcpConnectionContext> connectionCtx, const Error& ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("onNewTcpConnection: {}/{}",ec.code(),ec.message()));
        if (!ec)
        {
            connections[std::string{connectionCtx->id()}]=connectionCtx;

            auto& connection=connectionCtx->get<server::PlainTcpConnection>();
            server->handleNewConnection(connectionCtx,connection);
        }
    };

    auto tcpServerCtx=server::makePlainTcpServerContext(thread,"tcpserver");
    auto serverEnv=HATN_COMMON_NAMESPACE::makeEnvType<server::SimpleEnv>(
        HATN_COMMON_NAMESPACE::contexts(
            HATN_COMMON_NAMESPACE::context(thread),
            HATN_COMMON_NAMESPACE::context()
        )
    );
    serverEnv->get<HATN_COMMON_NAMESPACE::WithMappedThreads>().threads()->setDefaultThread(thread);
    auto& tcpServer=tcpServerCtx->get<server::PlainTcpServer>();
    tcpServer.setEnv(serverEnv);
    tcpServer.setConnectionHandler(onNewTcpConnection);
    asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
    auto ec=tcpServer.run(tcpServerCtx,serverEp);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("failed to listen: {}/{}",ec.code(),ec.message()));
    }
    BOOST_REQUIRE(!ec);

    return std::make_pair(server,tcpServerCtx);
}

BOOST_AUTO_TEST_SUITE(TestClientServer)

BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    createThreads(2);
    auto workThread=threadWithContextTask(1);

    std::ignore=createClient(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,TestEnv)
{
    createThreads(2);
    auto workThread=threadWithContextTask(1);

    std::map<std::string,SharedPtr<server::PlainTcpConnectionContext>> connections;
    std::ignore=createServer(workThread.get(),connections);

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

