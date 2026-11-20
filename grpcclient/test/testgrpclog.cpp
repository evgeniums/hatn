/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/test/testgrpclog.cpp
  *
  *  Bookkeeping of the gRPC log sink and scoped tracers. Needs no server and no logger:
  *  with no logger the sink drops the messages, the level handling is the same.
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/grpcclient/grpclog.h>

namespace grpcclient=HATN_GRPCCLIENT_NAMESPACE;
namespace hlog=HATN_LOGCONTEXT_NAMESPACE;

BOOST_AUTO_TEST_SUITE(TestGrpcLog)

BOOST_AUTO_TEST_CASE(LevelFollowsTraceScopes)
{
    grpcclient::installGrpcLogSink(std::shared_ptr<hlog::Logger>{},hlog::LogLevel::Warn);
    // HATN_GRPC_TRACE in the environment may have raised the base level at install
    grpcclient::setGrpcLogLevel(hlog::LogLevel::Warn);
    BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Warn);

    {
        grpcclient::ScopedGrpcTrace outer{std::vector<std::string>{"tcp"}};
        BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Info);

        {
            // overlapping scope that repeats a tracer of the outer one
            grpcclient::ScopedGrpcTrace inner{std::vector<std::string>{"tcp","subchannel"}};
            BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Info);
        }

        // the outer scope is still alive
        BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Info);
    }
    BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Warn);

    grpcclient::uninstallGrpcLogSink();
}

BOOST_AUTO_TEST_CASE(TraceNeverLowersVerbosity)
{
    grpcclient::installGrpcLogSink(std::shared_ptr<hlog::Logger>{},hlog::LogLevel::Debug);
    BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Debug);

    {
        grpcclient::ScopedGrpcTrace scope{std::vector<std::string>{"subchannel"}};
        BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Debug);
    }
    BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Debug);

    grpcclient::uninstallGrpcLogSink();
}

BOOST_AUTO_TEST_CASE(InstallUninstallRepeated)
{
    // no sink installed: nothing to do
    grpcclient::uninstallGrpcLogSink();
    grpcclient::uninstallGrpcLogSink();

    // a scope may outlive the sink
    grpcclient::installGrpcLogSink(std::shared_ptr<hlog::Logger>{},hlog::LogLevel::Error);
    {
        grpcclient::ScopedGrpcTrace scope{std::vector<std::string>{"tcp"}};
        grpcclient::uninstallGrpcLogSink();
    }

    // and the sink can be installed again
    grpcclient::installGrpcLogSink(std::shared_ptr<hlog::Logger>{},hlog::LogLevel::Error);
    grpcclient::setGrpcLogLevel(hlog::LogLevel::Error);
    BOOST_CHECK(grpcclient::grpcLogLevel()==hlog::LogLevel::Error);
    grpcclient::uninstallGrpcLogSink();
}

BOOST_AUTO_TEST_CASE(DefaultTracers)
{
    const auto tracers=grpcclient::defaultGrpcDiagnosticTracers();
    BOOST_CHECK(!tracers.empty());

    // the per-frame tracers are too noisy for a diagnostic run
    for (const auto& name : tracers)
    {
        BOOST_CHECK(name!="http");
        BOOST_CHECK(name!="secure_endpoint");
    }
}

BOOST_AUTO_TEST_SUITE_END()
