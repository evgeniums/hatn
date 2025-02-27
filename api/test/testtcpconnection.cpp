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

#include <hatn/api/api.h>
#include <hatn/api/client/client.h>
#include <hatn/api/client/tcpconnection.h>
#include <hatn/api/server/tcpserver.h>

// HATN_API_USING
// HATN_COMMON_USING
// HATN_LOGCONTEXT_USING

namespace {

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
    }

    ~Env()
    {
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

constexpr const uint32_t TcpPort=53852;

auto createClient(HATN_COMMON_NAMESPACE::Thread* thread)
{
    auto resolver=std::make_shared<HATN_API_NAMESPACE::client::IpHostResolver>(thread);
    std::vector<HATN_API_NAMESPACE::client::IpHostName> hosts;
    hosts.resize(1);
    hosts[0].name="localhost";
    hosts[0].port=TcpPort;
    hosts[0].ipVersion=HATN_NETWORK_NAMESPACE::IpVersion::V4;

    auto connectionCtx=HATN_COMMON_NAMESPACE::makeTaskContext<
        HATN_API_NAMESPACE::client::TcpClient,
        HATN_API_NAMESPACE::client::TcpConnection,
        HATN_LOGCONTEXT_NAMESPACE::Context
        >(
            HATN_COMMON_NAMESPACE::subcontexts(
                HATN_COMMON_NAMESPACE::subcontext(hosts,resolver,thread),
                HATN_COMMON_NAMESPACE::subcontext(),
                HATN_COMMON_NAMESPACE::subcontext()
            ),
            "client"
        );
    auto& tcpClient=connectionCtx->get<HATN_API_NAMESPACE::client::TcpClient>();
    auto& connection=connectionCtx->get<HATN_API_NAMESPACE::client::TcpConnection>();
    connection.setStreams(&tcpClient);

    return connectionCtx;
}

auto createServer(HATN_COMMON_NAMESPACE::Thread* thread)
{
    return HATN_COMMON_NAMESPACE::makeTaskContext<
        HATN_API_NAMESPACE::server::PlainTcpServer,
        HATN_LOGCONTEXT_NAMESPACE::Context
        >(
            HATN_COMMON_NAMESPACE::subcontexts(
                HATN_COMMON_NAMESPACE::subcontext(thread),
                HATN_COMMON_NAMESPACE::subcontext()
            ),
            "server"
        );
}

BOOST_AUTO_TEST_SUITE(TestTcpConnection)

BOOST_FIXTURE_TEST_CASE(CreateClient,Env)
{
    createThreads(2);
    auto workThread=thread(1);

    std::ignore=createClient(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,Env)
{
    createThreads(2);
    auto workThread=thread(1);

    std::ignore=createServer(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(ConnectClientServer,Env)
{
    createThreads(2);
    auto serverThread=thread(0);
    auto clientThread=thread(1);

    auto serverCtx=createServer(serverThread.get());
    auto clientCtx=createClient(clientThread.get());

    serverThread->start();
    clientThread->start();

    std::atomic<size_t> clientConnectCount{0};
    std::atomic<size_t> serverConnectCount{0};

    auto serverCb=[&serverConnectCount](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainConnectionContext> ctx, const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverCb: {}/{}",ec.code(),ec.message()));
        if (!ec)
        {
            serverConnectCount++;
        }
    };

    auto& server=serverCtx->get<HATN_API_NAMESPACE::server::PlainTcpServer>();
    server.setConnectionHandler(serverCb);
    HATN_NETWORK_NAMESPACE::asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
    auto ec=server.run(serverCtx,serverEp);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("failed to listen: {}/{}",ec.code(),ec.message()));
    }
    BOOST_REQUIRE(!ec);

    auto clientCb=[&clientConnectCount](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientCb: {}/{}",ec.code(),ec.message()));

        if (!ec)
        {
            clientConnectCount++;
        }
        else
        {
            HATN_CHECK_TS(false);
        }
    };
    auto& client=clientCtx->get<HATN_API_NAMESPACE::client::TcpConnection>();
    client.connect(clientCb);

    auto clientCtx2=createClient(clientThread.get());
    auto& client2=clientCtx2->get<HATN_API_NAMESPACE::client::TcpConnection>();
    client2.connect(clientCb);

    exec(1);

    std::ignore=server.close();

    serverThread->stop();
    clientThread->stop();

    exec(1);

    BOOST_CHECK_EQUAL(serverConnectCount,2);
    BOOST_CHECK_EQUAL(clientConnectCount,2);
}

BOOST_AUTO_TEST_SUITE_END()
