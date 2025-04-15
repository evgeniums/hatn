/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file serveradmin/test/testadmin.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/app/appname.h>
#include <hatn/app/baseapp.h>

#include <hatn/adminserver/apiadmincontroller.h>
#include <hatn/adminserver/appadmincontroller.h>
#include <hatn/adminserver/ipp/localadmincontroller.ipp>

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

/********************** Tests **************************/

BOOST_AUTO_TEST_SUITE(TestAdminOps)

BOOST_FIXTURE_TEST_CASE(InitApiController,TestEnv)
{
    HATN_ADMIN_SERVER_NAMESPACE::ApiAdminController ctrl{"system"};

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(InitAppController,TestEnv)
{
    HATN_APP_NAMESPACE::BaseApp app{HATN_APP_NAMESPACE::AppName{"testapp","Test App"}};

    HATN_ADMIN_SERVER_NAMESPACE::AppAdminController ctrl{app.env(),"system"};

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()

