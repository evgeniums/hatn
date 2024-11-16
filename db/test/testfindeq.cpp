/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testfindeq.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "models9.h"
#include "findhandlers.h"

namespace {

constexpr const size_t count=100;

struct noPartitionT
{
    template <typename T>
    void operator()(T&&, size_t) const noexcept
    {
    }
};
constexpr noPartitionT noPartition{};

uint8_t genUInt8(size_t i)
{
    return static_cast<uint8_t>(i);
}

Topic topic()
{
    return "topic1";
}

}

BOOST_AUTO_TEST_SUITE(TestFindEq, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(CheckEqInt)
{
    init();
    registerModels9();
    auto s1=initSchema(m9());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        m9()->model.indexId(u9_f2_idx());

        fillDbForFind(count,client,topic(),m9(),genUInt8,noPartition,u9::f6);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
