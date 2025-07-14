/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/test/testapp.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/app/appenv.h>

#include "hatn_test_config.h"

HATN_USING
HATN_APP_USING

class TestContext
{
    public:
};

using TestEnv=common::EnvTmpl<EnvWithAppEnvT,TestContext>;

BOOST_AUTO_TEST_SUITE(TestApp)

BOOST_AUTO_TEST_CASE(AppEnv)
{
    TestEnv env;
    auto appEnv=common::makeShared<HATN_APP_NAMESPACE::AppEnv>();
    env.setEmbeddedEnv(std::move(appEnv));

    auto& testCtx=env.get<TestContext>();
    static_assert(std::is_same<TestContext,std::decay_t<decltype(testCtx)>>::value,"");

    auto hasTestCtx=env.hasContext<TestContext>();
    static_assert(decltype(hasTestCtx)::value,"");

    auto& factoryCtx=env.get<AllocatorFactory>();
    static_assert(std::is_same<AllocatorFactory,std::decay_t<decltype(factoryCtx)>>::value,"");

    auto hasFactoryCtx=env.hasContext<AllocatorFactory>();
    static_assert(decltype(hasFactoryCtx)::value,"");
}

BOOST_AUTO_TEST_SUITE_END()
