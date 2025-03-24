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

HDU_UNIT_WITH(vec,(HDU_BASE(object)),
    HDU_ENUM(MyEnum,One=1,Two=2,Three=3)
    HDU_REPEATED_FIELD(f1,TYPE_BOOL,1)
    HDU_REPEATED_FIELD(f2,TYPE_INT8,2)
    HDU_REPEATED_FIELD(f3,TYPE_INT16,3)
    HDU_REPEATED_FIELD(f4,TYPE_INT32,4)
    HDU_REPEATED_FIELD(f5,TYPE_INT64,5)
    HDU_REPEATED_FIELD(f6,TYPE_UINT8,6)
    HDU_REPEATED_FIELD(f7,TYPE_UINT16,7)
    HDU_REPEATED_FIELD(f8,TYPE_UINT32,8)
    HDU_REPEATED_FIELD(f9,TYPE_UINT64,9)
    HDU_REPEATED_FIELD(f10,TYPE_STRING,10)
    HDU_REPEATED_FIELD(f11,HDU_TYPE_ENUM(MyEnum),11)
    HDU_REPEATED_FIELD(f12,HDU_TYPE_FIXED_STRING(64),12)
    HDU_REPEATED_FIELD(f13,TYPE_DATETIME,13)
    HDU_REPEATED_FIELD(f14,TYPE_DATE,14)
    HDU_REPEATED_FIELD(f15,TYPE_TIME,15)
    HDU_REPEATED_FIELD(f16,TYPE_OBJECT_ID,16)
    HDU_REPEATED_FIELD(f17,TYPE_DATE_RANGE,17)
    HDU_REPEATED_FIELD(f18,TYPE_BYTES,18)
    HDU_REPEATED_FIELD(f19,TYPE_FLOAT,19)
    HDU_REPEATED_FIELD(f20,TYPE_DOUBLE,20)
)


BOOST_AUTO_TEST_SUITE(TestUpdateSerialize)

BOOST_AUTO_TEST_CASE(OneFieldSerDeser)
{
    common::pmr::vector<update::serialization::VectorsHolder> vectorsHolder;
    auto r1=update::request(
       update::field(plain::f2,update::set,100)
    );

    update::message::type msg1;
    auto ec=update::serialize(r1,msg1);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg1: {}",msg1.toString(true,1)));

    update::Request r2;
    auto res=update::deserialize(msg1,r2,vectorsHolder);
    BOOST_CHECK(!res);
    update::message::type msg2;
    ec=update::serialize(r2,msg2);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(msg1.toString(),msg2.toString());
}

BOOST_AUTO_TEST_CASE(ScalarSetUnset)
{
    common::pmr::vector<update::serialization::VectorsHolder> vectorsHolder;
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
    auto res=update::deserialize(msg1,r2,vectorsHolder);
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
    res=update::deserialize(msg3,r3_,vectorsHolder);
    BOOST_CHECK(!res);
    update::apply(&obj,r3_);
    BOOST_TEST_MESSAGE(fmt::format("after apply unset: {}",obj.toString(true,4)));
    std::string sample1="{\"f1\":true,\"f2\":100,\"f3\":1000,\"f4\":10000,\"f5\":100000,\"f6\":200,\"f7\":2000,\"f8\":20000,\"f9\":200000,\"f11\":3,\"f12\":\"hi!\",\"f13\":\"2024-03-24T13:32:19.432Z\",\"f14\":\"2024-03-24\",\"f15\":\"13:32:19\",\"f16\":\"195c85aa86900000190592bde\",\"f17\":\"32024003\",\"f18\":\"SGkgYnl0ZXMh\",\"f19\":101.202,\"f20\":1001.2002}";
    BOOST_CHECK_EQUAL(sample1,obj.toString(false,4));
}

BOOST_AUTO_TEST_CASE(VectorOps)
{
    common::pmr::vector<update::serialization::VectorsHolder> vectorsHolder;

    auto dt=common::DateTime::parseIsoString("2024-03-24T13:32:19.432Z");
    auto oid=ObjectId{};
    oid.set("195c85aa86900000190592bde");
    auto r1=update::request(
        update::field(vec::f1,update::push,true),
        update::field(vec::f2,update::push,100),
        update::field(vec::f3,update::push,1000),
        update::field(vec::f4,update::push,10000),
        update::field(vec::f5,update::push,100000),
        update::field(vec::f6,update::push,200),
        update::field(vec::f7,update::push,2000),
        update::field(vec::f8,update::push,20000),
        update::field(vec::f9,update::push,200000),
        update::field(vec::f10,update::push,"hello world"),
        update::field(vec::f11,update::push,plain::MyEnum::Three),
        update::field(vec::f12,update::push,"hi!"),
        update::field(vec::f13,update::push,dt.value()),
        update::field(vec::f14,update::push,dt->date()),
        update::field(vec::f15,update::push,dt->time()),
        update::field(vec::f16,update::push,oid),
        update::field(vec::f17,update::push,common::DateRange{dt.value()}),
        update::field(vec::f18,update::push,"Hi bytes!"),
        update::field(vec::f19,update::push,101.202),
        update::field(vec::f20,update::push,1001.2002)
    );

    update::message::type msg1;
    auto ec=update::serialize(r1,msg1);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg1: {}",msg1.toString(true,4)));
    update::Request r2;
    auto res=update::deserialize(msg1,r2,vectorsHolder);
    BOOST_CHECK(!res);
    update::message::type msg2;
    ec=update::serialize(r2,msg2);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(msg1.toString(),msg2.toString());

    vec::type obj;
    update::apply(&obj,r2);
    auto str=obj.toString(true,4);
    BOOST_TEST_MESSAGE(fmt::format("after apply: {}",str));

    auto dt2=common::DateTime::parseIsoString("2024-04-24T15:30:20.107Z");
    auto oid2=ObjectId{};
    oid2.set("195c85aa86900000290592bde");
    auto r3=update::request(
        update::field(vec::f1,update::push_unique,false),
        update::field(vec::f2,update::push_unique,101),
        update::field(vec::f3,update::push_unique,1001),
        update::field(vec::f4,update::push_unique,10001),
        update::field(vec::f5,update::push_unique,100001),
        update::field(vec::f6,update::push_unique,201),
        update::field(vec::f7,update::push_unique,2001),
        update::field(vec::f8,update::push_unique,20001),
        update::field(vec::f9,update::push_unique,200001),
        update::field(vec::f10,update::push_unique,"hello world 1"),
        update::field(vec::f11,update::push_unique,plain::MyEnum::Two),
        update::field(vec::f12,update::push_unique,"hi 1!"),
        update::field(vec::f13,update::push_unique,dt2.value()),
        update::field(vec::f14,update::push_unique,dt2->date()),
        update::field(vec::f15,update::push_unique,dt2->time()),
        update::field(vec::f16,update::push_unique,oid2),
        update::field(vec::f17,update::push_unique,common::DateRange{dt2.value()}),
        update::field(vec::f18,update::push_unique,"Hi bytes 1!"),
        update::field(vec::f20,update::push_unique,1002.2002)
    );

    update::message::type msg3;
    ec=update::serialize(r3,msg3);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("msg2: {}",msg3.toString(true,4)));
    update::Request r4;
    res=update::deserialize(msg3,r4,vectorsHolder);
    BOOST_CHECK(!res);
    BOOST_CHECK(!ec);
    update::apply(&obj,r4);
    auto str2=obj.toString(true,4);
    BOOST_TEST_MESSAGE(fmt::format("after apply 2: {}",str2));
    std::string sample1="{\"f1\":[true,false],\"f2\":[100,101],\"f3\":[1000,1001],\"f4\":[10000,10001],\"f5\":[100000,100001],\"f6\":[200,201],\"f7\":[2000,2001],\"f8\":[20000,20001],\"f9\":[200000,200001],\"f10\":[\"hello world\",\"hello world 1\"],\"f11\":[3,2],\"f12\":[\"hi!\",\"hi 1!\"],\"f13\":[\"2024-03-24T13:32:19.432Z\",\"2024-04-24T15:30:20.107Z\"],\"f14\":[\"2024-03-24\",\"2024-04-24\"],\"f15\":[\"13:32:19\",\"15:30:20\"],\"f16\":[\"195c85aa86900000190592bde\",\"195c85aa86900000290592bde\"],\"f17\":[\"32024003\",\"32024004\"],\"f18\":[\"SGkgYnl0ZXMh\",\"SGkgYnl0ZXMgMSE=\"],\"f19\":[101.202],\"f20\":[1001.2002,1002.2002]}";
    BOOST_CHECK_EQUAL(sample1,obj.toString(false,4));

    update::apply(&obj,r4);
    auto str3=obj.toString(true,4);
    BOOST_TEST_MESSAGE(fmt::format("after apply 3: {}",str3));
    BOOST_CHECK_EQUAL(str2,str3);

    r3=update::request(
        update::field(vec::f4,update::push,50010),
        update::field(vec::f8,update::push,70020),
        update::field(vec::f10,update::push_unique,"hello world again!")
    );
    ec=update::serialize(r3,msg3);
    BOOST_CHECK(!ec);
    res=update::deserialize(msg3,r4,vectorsHolder);
    update::apply(&obj,r4);
    auto str4=obj.toString(true,4);
    BOOST_TEST_MESSAGE(fmt::format("after apply 4: {}",str4));
    auto sample4="{\"f1\":[true,false],\"f2\":[100,101],\"f3\":[1000,1001],\"f4\":[10000,10001,50010],\"f5\":[100000,100001],\"f6\":[200,201],\"f7\":[2000,2001],\"f8\":[20000,20001,70020],\"f9\":[200000,200001],\"f10\":[\"hello world\",\"hello world 1\",\"hello world again!\"],\"f11\":[3,2],\"f12\":[\"hi!\",\"hi 1!\"],\"f13\":[\"2024-03-24T13:32:19.432Z\",\"2024-04-24T15:30:20.107Z\"],\"f14\":[\"2024-03-24\",\"2024-04-24\"],\"f15\":[\"13:32:19\",\"15:30:20\"],\"f16\":[\"195c85aa86900000190592bde\",\"195c85aa86900000290592bde\"],\"f17\":[\"32024003\",\"32024004\"],\"f18\":[\"SGkgYnl0ZXMh\",\"SGkgYnl0ZXMgMSE=\"],\"f19\":[101.202],\"f20\":[1001.2002,1002.2002]}";
    BOOST_CHECK_EQUAL(sample4,obj.toString(false,4));

    std::vector<int16_t> setV3{900,901,902,903};
    std::vector<int16_t> setV5{9000,9001,9002,9003,9004};
    std::vector<std::string> setV12{"aaaa","aabb","bbcc"};
    r3=update::request(
        update::field(update::path(array(vec::f4,1)),update::inc_element,210),
        update::field(update::path(array(vec::f8,0)),update::replace_element,777),
        update::field(update::path(array(vec::f10,1)),update::erase_element),
        update::field(vec::f9,update::pop),
        update::field(vec::f3,update::set,setV3),
        update::field(vec::f5,update::set,setV5),
        update::field(vec::f12,update::set,setV12)
    );
    ec=update::serialize(r3,msg3);
    BOOST_CHECK(!ec);
    BOOST_TEST_MESSAGE(fmt::format("mixed update: {}",msg3.toString(true,4)));
    res=update::deserialize(msg3,r4,vectorsHolder);
    BOOST_REQUIRE_EQUAL(vectorsHolder.size(),3);
    const auto& v3hVar=vectorsHolder.at(0);
    const auto& v3h=lib::variantGet<common::pmr::vector<int16_t>>(v3hVar);
    const auto* ptr1=&v3h;
    BOOST_REQUIRE_EQUAL(v3h.size(),4);
    BOOST_REQUIRE_EQUAL(r4.size(),7);
    const auto& f3Var=r4.at(4);
    const auto& f3Var1=f3Var.value.as<Vector<int16_t>>();
    const auto& f3Val=lib::variantGet<std::reference_wrapper<const common::pmr::vector<int16_t>>>(f3Var1);
    const auto* ptr2=&(f3Val.get());
    BOOST_REQUIRE_EQUAL(ptr1,ptr2);
    BOOST_REQUIRE_EQUAL(f3Val.get().size(),4);
    update::apply(&obj,r4);
    auto str5=obj.toString(true,4);
    BOOST_TEST_MESSAGE(fmt::format("after apply 5: {}",str5));
    auto sample5="{\"f1\":[true,false],\"f2\":[100,101],\"f3\":[900,901,902,903],\"f4\":[10000,10211,50010],\"f5\":[9000,9001,9002,9003,9004],\"f6\":[200,201],\"f7\":[2000,2001],\"f8\":[777,20001,70020],\"f9\":[200000],\"f10\":[\"hello world\",\"hello world again!\"],\"f11\":[3,2],\"f12\":[\"aaaa\",\"aabb\",\"bbcc\"],\"f13\":[\"2024-03-24T13:32:19.432Z\",\"2024-04-24T15:30:20.107Z\"],\"f14\":[\"2024-03-24\",\"2024-04-24\"],\"f15\":[\"13:32:19\",\"15:30:20\"],\"f16\":[\"195c85aa86900000190592bde\",\"195c85aa86900000290592bde\"],\"f17\":[\"32024003\",\"32024004\"],\"f18\":[\"SGkgYnl0ZXMh\",\"SGkgYnl0ZXMgMSE=\"],\"f19\":[101.202],\"f20\":[1001.2002,1002.2002]}";
    BOOST_CHECK_EQUAL(sample5,obj.toString(false,4));

    //! @todo check vetor set of all types
    //! @todo check inc of scalar types
}

BOOST_AUTO_TEST_SUITE_END()
