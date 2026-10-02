/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file dataunit/test/testwirejson.cpp
  *
  *     Schemaless wire-to-JSON transcoding (wirejson.h).
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include <hatn/dataunit/wirejson.h>

namespace {

HDU_UNIT(wj_inner,
    HDU_FIELD(id,TYPE_UINT32,1)
    HDU_FIELD(label,TYPE_STRING,2)
)

HDU_UNIT(wj_outer,
    HDU_FIELD(count,TYPE_UINT32,1)
    HDU_FIELD(title,TYPE_STRING,2)
    HDU_FIELD(blob,TYPE_BYTES,3)
    HDU_FIELD(inner,wj_inner::TYPE,4)
    HDU_REPEATED_FIELD(tags,TYPE_STRING,5)
    HDU_FIELD(big,TYPE_UINT64,6)
)

namespace du=HATN_DATAUNIT_NAMESPACE;

template <typename T>
std::string serialize(T& obj)
{
    du::WireBufSolid buf;
    auto r=du::io::serialize(obj,buf);
    BOOST_REQUIRE(r>0);
    return std::string{buf.mainContainer()->data(),buf.mainContainer()->size()};
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestWireJson)

//! Numbers, text, binary, a nested message and a repeated field, keyed by field number.
BOOST_AUTO_TEST_CASE(TranscodesWithoutSchema)
{
    wj_outer::type obj;
    obj.setFieldValue(wj_outer::count,300u);
    obj.setFieldValue(wj_outer::title,std::string{"hello"});
    const char binary[]={'\x00','\x01','\x02','\xff'};
    obj.field(wj_outer::blob).set(binary,sizeof(binary));
    obj.field(wj_outer::inner).mutableValue()->setFieldValue(wj_inner::id,7u);
    obj.field(wj_outer::inner).mutableValue()->setFieldValue(wj_inner::label,std::string{"x y"});
    obj.field(wj_outer::tags).appendValue(std::string{"a b"});
    obj.field(wj_outer::tags).appendValue(std::string{"c d"});
    obj.setFieldValue(wj_outer::big,uint64_t(1)<<40);

    auto wire=serialize(obj);
    auto json=du::wireToJson(wire.data(),wire.size());
    BOOST_REQUIRE(!json);
    BOOST_TEST_MESSAGE(json.value());

    const auto& s=json.value();
    BOOST_CHECK(s.find("\"1\":300")!=std::string::npos);
    BOOST_CHECK(s.find("\"2\":\"hello\"")!=std::string::npos);
    BOOST_CHECK(s.find("\"3\":\"AAEC/w==\"")!=std::string::npos);
    BOOST_CHECK(s.find("\"4\":{\"1\":7,\"2\":\"x y\"}")!=std::string::npos);
    BOOST_CHECK(s.find("\"5\":[\"a b\",\"c d\"]")!=std::string::npos);
    BOOST_CHECK(s.find("\"6\":1099511627776")!=std::string::npos);
}

BOOST_AUTO_TEST_CASE(EmptyBlobIsAnEmptyObject)
{
    auto json=du::wireToJson(nullptr,0);
    BOOST_REQUIRE(!json);
    BOOST_CHECK_EQUAL(json.value(),"{}");
}

//! A truncated value or an invalid wire type is an error, not a partial result.
BOOST_AUTO_TEST_CASE(MalformedBlobIsAnError)
{
    // field 1, VarInt, value cut off
    const char truncated[]={'\x08','\x80'};
    BOOST_CHECK(du::wireToJson(truncated,sizeof(truncated)));

    // field 1, wire type 7 (reserved)
    const char badType[]={'\x0f','\x01'};
    BOOST_CHECK(du::wireToJson(badType,sizeof(badType)));

    // field 1, length 5 but only 2 bytes follow
    const char shortLength[]={'\x0a','\x05','a','b'};
    BOOST_CHECK(du::wireToJson(shortLength,sizeof(shortLength)));
}

//! Deeper nesting than maxDepth is rendered as text or bytes instead of an object.
BOOST_AUTO_TEST_CASE(DepthLimit)
{
    wj_outer::type obj;
    obj.field(wj_outer::inner).mutableValue()->setFieldValue(wj_inner::id,7u);
    auto wire=serialize(obj);

    auto deep=du::wireToJson(wire.data(),wire.size(),8);
    BOOST_REQUIRE(!deep);
    BOOST_CHECK(deep.value().find("\"4\":{")!=std::string::npos);

    auto shallow=du::wireToJson(wire.data(),wire.size(),0);
    BOOST_REQUIRE(!shallow);
    BOOST_CHECK(shallow.value().find("\"4\":{")==std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
