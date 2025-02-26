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

BOOST_AUTO_TEST_SUITE(TestTcpConnection)

BOOST_FIXTURE_TEST_CASE(Create,Env)
{
    createThreads(2);
    auto workThread=thread(1);

    auto resolver=std::make_shared<HATN_API_NAMESPACE::client::IpHostResolver>(workThread.get());
    std::vector<HATN_API_NAMESPACE::client::IpHostName> hosts;
    hosts.resize(1);
    hosts[0].name="localhost";
    hosts[0].port=8080;

    auto connectionCtx=HATN_COMMON_NAMESPACE::makeTaskContext<
            HATN_API_NAMESPACE::client::TcpClient,
            HATN_API_NAMESPACE::client::TcpConnection,
            HATN_LOGCONTEXT_NAMESPACE::Context
        >(
            HATN_COMMON_NAMESPACE::subcontexts(
                HATN_COMMON_NAMESPACE::subcontext(hosts,resolver,workThread.get()),
                HATN_COMMON_NAMESPACE::subcontext(),
                HATN_COMMON_NAMESPACE::subcontext()
            )
        );
    auto& tcpClient=connectionCtx->get<HATN_API_NAMESPACE::client::TcpClient>();
    auto& connection=connectionCtx->get<HATN_API_NAMESPACE::client::TcpConnection>();
    connection.setStreams(&tcpClient);
    std::ignore=connection;

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
