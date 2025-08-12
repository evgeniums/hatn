#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#define HDU_V2_UNIT_EXPORT

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/fields/map.h>

#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_DATAUNIT_USING

namespace {

HDU_MAP(MapItem,TYPE_STRING,TYPE_INT32)

HDU_UNIT(u1,
    HDU_MAP_FIELD(f1,MapItem::TYPE,1)
)

HDU_UNIT(u2,
    HDU_REPEATED_FIELD(f1,MapItem::TYPE,1)
)

HDU_MAP(MapItemStr,TYPE_UINT64,TYPE_STRING)

HDU_UNIT(u3,
         HDU_MAP_FIELD(f1,MapItemStr::TYPE,1)
         )

HDU_UNIT(u4,
         HDU_REPEATED_FIELD(f1,MapItemStr::TYPE,1)
         )

struct MapName1
{
    constexpr static const char* name="map1";
};

}

BOOST_AUTO_TEST_SUITE(TestMap)

BOOST_AUTO_TEST_CASE(TestMeta)
{
    std::ignore=MapName1::name;

    MapField<MapName1,MapItem::TYPE,1>* m1Ptr=nullptr;
    MapField<MapName1,MapItem::TYPE,1>& m1=*m1Ptr;
    std::ignore=m1;

    using Type=MapItem::TYPE;

    using pseudoSubunitType=typename Type::type;

    using keyFieldType=typename pseudoSubunitType::template fieldType<0>;
    using valueFieldType=typename pseudoSubunitType::template fieldType<1>;

    using keyType=typename detail::MapVariableType<typename keyFieldType::Type>::type;
    using valueType=typename detail::MapVariableType<typename valueFieldType::Type>::type;

    static_assert(std::is_same<keyType,std::string>::value,"");
    static_assert(std::is_same<valueType,int32_t>::value,"");

    using keyTypeId=typename keyFieldType::Type;
    using valueTypeId=typename valueFieldType::Type;

    static_assert(std::is_same<keyTypeId,types::TYPE_STRING>::value,"");
    static_assert(std::is_same<valueTypeId,types::TYPE_INT32>::value,"");

    using type=HATN_COMMON_NAMESPACE::pmr::map<keyType,valueType>;
    static_assert(std::is_same<type,MapField<MapName1,MapItem::TYPE,1>::type>::value,"");

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(TestMapField)
{
    u1::type o1;

    auto json1=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Object with empty map:\n{}\n",json1));

    o1.field(u1::f1).setValue("key1",100);
    o1.field(u1::f1).setValue("key2",200);
    o1.field(u1::f1).setValue("key3",300);

    BOOST_CHECK_EQUAL(o1.field(u1::f1).at("key1"),100);
    BOOST_CHECK_EQUAL(o1.field(u1::f1).at("key2"),200);
    BOOST_CHECK_EQUAL(o1.field(u1::f1).at("key3"),300);

    auto json2=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Object with filled map:\n{}\n",json2));

    bool ok=true;
    WireBufSolid buf1;
    ok=io::serialize(o1,buf1);
    BOOST_REQUIRE(ok);

    u2::type o1_r;
    auto& item1=o1_r.field(u2::f1).appendSharedSubunit();
    item1.setFieldValue(MapItem::key,"key1");
    item1.setFieldValue(MapItem::value,100);
    auto& item2=o1_r.field(u2::f1).appendSharedSubunit();
    item2.setFieldValue(MapItem::key,"key2");
    item2.setFieldValue(MapItem::value,200);
    auto& item3=o1_r.field(u2::f1).appendSharedSubunit();
    item3.setFieldValue(MapItem::key,"key3");
    item3.setFieldValue(MapItem::value,300);

    WireBufSolid buf1_r;
    ok=io::serialize(o1_r,buf1_r);
    BOOST_REQUIRE(ok);

    u1::type o2;
    HATN_NAMESPACE::Error ec;
    ok=io::deserialize(o2,buf1,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json3=o2.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object from object map field:\n{}\n",json3));

    u1::type o3;
    ok=io::deserialize(o3,buf1_r,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json4=o3.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object from object with repeated field:\n{}\n",json4));

    u2::type o3_r;
    ok=io::deserialize(o3_r,buf1,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json5=o3_r.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object with repeated field from object with map field:\n{}\n",json5));

    u2::type o2_r;
    ok=io::deserialize(o2_r,buf1_r,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json6=o2_r.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object with repeated field from object with repeated field:\n{}\n",json6));

    BOOST_CHECK_EQUAL(json2,json3);
    BOOST_CHECK_EQUAL(json3,json4);
    BOOST_CHECK_EQUAL(json4,json5);
    BOOST_CHECK_EQUAL(json5,json6);
}

BOOST_AUTO_TEST_CASE(TestStringMap)
{
    u3::type o1;

    auto json1=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Object with empty map:\n{}\n",json1));

    o1.field(u3::f1).setValue(100,"val1");
    o1.field(u3::f1).setValue(200,"val2");
    o1.field(u3::f1).setValue(300,"val3");

    BOOST_CHECK_EQUAL(o1.field(u3::f1).at(100),"val1");
    BOOST_CHECK_EQUAL(o1.field(u3::f1).at(200),"val2");
    BOOST_CHECK_EQUAL(o1.field(u3::f1).at(300),"val3");

    auto json2=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Object with filled map:\n{}\n",json2));

    bool ok=true;
    WireBufSolid buf1;
    ok=io::serialize(o1,buf1);
    BOOST_REQUIRE(ok);

    u4::type o1_r;
    auto& item1=o1_r.field(u4::f1).appendSharedSubunit();
    item1.setFieldValue(MapItemStr::key,100);
    item1.setFieldValue(MapItemStr::value,"val1");
    auto& item2=o1_r.field(u4::f1).appendSharedSubunit();
    item2.setFieldValue(MapItemStr::key,200);
    item2.setFieldValue(MapItemStr::value,"val2");
    auto& item3=o1_r.field(u4::f1).appendSharedSubunit();
    item3.setFieldValue(MapItemStr::key,300);
    item3.setFieldValue(MapItemStr::value,"val3");

    WireBufSolid buf1_r;
    ok=io::serialize(o1_r,buf1_r);
    BOOST_REQUIRE(ok);

    u3::type o2;
    HATN_NAMESPACE::Error ec;
    ok=io::deserialize(o2,buf1,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json3=o2.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object from object map field:\n{}\n",json3));

    u3::type o3;
    ok=io::deserialize(o3,buf1_r,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json4=o3.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object from object with repeated field:\n{}\n",json4));

    u4::type o3_r;
    ok=io::deserialize(o3_r,buf1,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json5=o3_r.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object with repeated field from object with map field:\n{}\n",json5));

    u4::type o2_r;
    ok=io::deserialize(o2_r,buf1_r,ec);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());
    }
    BOOST_REQUIRE(ok);
    auto json6=o2_r.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("Deserialized object with repeated field from object with repeated field:\n{}\n",json6));

    BOOST_CHECK_EQUAL(json2,json3);
    BOOST_CHECK_EQUAL(json3,json4);
    BOOST_CHECK_EQUAL(json4,json5);
    BOOST_CHECK_EQUAL(json5,json6);
}

BOOST_AUTO_TEST_SUITE_END()
