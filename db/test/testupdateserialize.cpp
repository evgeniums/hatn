/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testupdate.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/db/schema.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>
#include <hatn/db/ipp/updateserialization.ipp>

#include "hatn_test_config.h"

namespace tt = boost::test_tools;

#include "modelplain.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {
} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestUpdateSerialize)

BOOST_AUTO_TEST_CASE(OneFieldSerDeser)
{
    auto r1=update::request(
       update::field(plain::f2,update::set,100)
    );

    update::message::type msg1;
    auto ec=update::serialize(r1,msg1);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg1: {}",msg1.toString(true,1)));

    update::Request r2;
    auto res=update::deserialize(msg1,r2);
    BOOST_CHECK(!res);
    update::message::type msg2;
    ec=update::serialize(r2,msg2);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(msg1.toString(),msg2.toString());
}

BOOST_AUTO_TEST_CASE(ScalarSetUnset)
{
    auto dt=common::DateTime::parseIsoString("2024-03-24T13:32:19.432Z");
    auto oid=ObjectId{};
    oid.set("195c85aa86900000190592bde");
    auto r1=update::request(
        update::field(plain::f1,update::set,true),
        update::field(plain::f2,update::set,100),
        update::field(plain::f3,update::set,1000),
        update::field(plain::f4,update::set,10000),
        update::field(plain::f5,update::set,100000),
        update::field(plain::f6,update::set,200),
        update::field(plain::f7,update::set,2000),
        update::field(plain::f8,update::set,20000),
        update::field(plain::f9,update::set,200000),
        update::field(plain::f10,update::set,"hello world"),
        update::field(plain::f11,update::set,plain::MyEnum::Three),
        update::field(plain::f12,update::set,"hi!"),
        update::field(plain::f13,update::set,dt.value()),
        update::field(plain::f14,update::set,dt->date()),
        update::field(plain::f15,update::set,dt->time()),
        update::field(plain::f16,update::set,oid),
        update::field(plain::f17,update::set,common::DateRange{dt.value()}),
        update::field(plain::f18,update::set,"Hi bytes!"),
        update::field(plain::f19,update::set,101.202),
        update::field(plain::f20,update::set,1001.2002)
    );

    update::message::type msg1;
    auto ec=update::serialize(r1,msg1);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg1: {}",msg1.toString(true,4)));

    update::Request r2;
    auto res=update::deserialize(msg1,r2);
    BOOST_CHECK(!res);
    update::message::type msg2;
    ec=update::serialize(r2,msg2);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(msg1.toString(),msg2.toString());

    plain::type obj;
    update::apply(&obj,r2);
    BOOST_TEST_MESSAGE(fmt::format("after apply: {}",obj.toString(true,4)));

    std::string sample="{\"f1\":true,\"f2\":100,\"f3\":1000,\"f4\":10000,\"f5\":100000,\"f6\":200,\"f7\":2000,\"f8\":20000,\"f9\":200000,\"f10\":\"hello world\",\"f11\":3,\"f12\":\"hi!\",\"f13\":\"2024-03-24T13:32:19.432Z\",\"f14\":\"2024-03-24\",\"f15\":\"13:32:19\",\"f16\":\"195c85aa86900000190592bde\",\"f17\":\"32024003\",\"f18\":\"SGkgYnl0ZXMh\",\"f19\":101.202,\"f20\":1001.2002}";
    BOOST_CHECK_EQUAL(sample,obj.toString(false,4));

    update::message::type msg3;
    auto r3=update::request(update::field(plain::f10,update::unset));
    ec=update::serialize(r3,msg3);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg3: {}",msg3.toString(true,4)));
    update::Request r3_;
    res=update::deserialize(msg3,r3_);
    BOOST_CHECK(!res);
    update::apply(&obj,r3_);
    BOOST_TEST_MESSAGE(fmt::format("after apply unset: {}",obj.toString(true,4)));
    std::string sample1="{\"f1\":true,\"f2\":100,\"f3\":1000,\"f4\":10000,\"f5\":100000,\"f6\":200,\"f7\":2000,\"f8\":20000,\"f9\":200000,\"f11\":3,\"f12\":\"hi!\",\"f13\":\"2024-03-24T13:32:19.432Z\",\"f14\":\"2024-03-24\",\"f15\":\"13:32:19\",\"f16\":\"195c85aa86900000190592bde\",\"f17\":\"32024003\",\"f18\":\"SGkgYnl0ZXMh\",\"f19\":101.202,\"f20\":1001.2002}";
    BOOST_CHECK_EQUAL(sample1,obj.toString(false,4));
}

//! @todo Test serialization of update operations on vector and subunit fields

BOOST_AUTO_TEST_SUITE_END()
