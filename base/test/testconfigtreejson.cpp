/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/test/testconfigtreejson.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/base/configtreejson.h>
#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_BASE_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(TestConfigTree)

BOOST_AUTO_TEST_CASE(ConfigTreeLoadJson)
{
    ConfigTree t1;

    ConfigTreeJson jsonIo;

    auto filename1=MultiThreadFixture::assetsFilePath("base/assets/config1.jsonc");
    auto ec=jsonIo.loadFile(t1,filename1);
    if (!ec.isNull())
    {
        BOOST_FAIL(ec.message());
    }
    BOOST_CHECK(!ec);

    auto jsonR1=jsonIo.serialize(t1);
    BOOST_CHECK(!jsonR1);
    std::cout<<"Parsed and serialized back object 1"<<std::endl;
    std::cout<<"*********************************"<<std::endl;
    std::cout<<jsonR1.value()<<std::endl;
    std::cout<<"*********************************"<<std::endl;

    auto filename2=MultiThreadFixture::assetsFilePath("base/assets/config_err1.jsonc");
    ec=jsonIo.loadFile(t1,filename2);
    BOOST_TEST_MESSAGE(fmt::format("Expected parsing failure: {}", ec.message()));
    BOOST_CHECK(ec);

    auto filename3=MultiThreadFixture::assetsFilePath("base/assets/config_err2.jsonc");
    ec=jsonIo.loadFile(t1,filename3);
    BOOST_TEST_MESSAGE(fmt::format("Expected parsing failure: {}", ec.message()));
    BOOST_CHECK(ec);    
}

BOOST_AUTO_TEST_SUITE_END()
