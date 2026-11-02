#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>
#include <hatn/dataunit/visitors.h>

HATN_USING
HATN_DATAUNIT_USING
using namespace HATN_DATAUNIT_NAMESPACE::types;
using namespace HATN_DATAUNIT_NAMESPACE::meta;

namespace {
#if 0
struct f0_name
{
    constexpr static const char* name="f0";
};
#endif
HDU_UNIT(u1,
    HDU_FIELD(f1,TYPE_DATETIME,1)
    HDU_FIELD(f2,TYPE_DATE,2)
    HDU_FIELD(f3,TYPE_TIME,3)
    HDU_FIELD(f4,TYPE_DATE_RANGE,4)
)

//! @todo Test get/set of date and time fields

}

BOOST_AUTO_TEST_SUITE(TestDatetimeField)

BOOST_AUTO_TEST_CASE(DatetimeFieldUtc)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    f1.mutableValue()->loadCurrentUtc();
    BOOST_TEST_MESSAGE(fmt::format("before serialize: {}",f1.value().toIsoString()));

    WireBufSolid buf;
    Error ec;
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf,ec);
    BOOST_REQUIRE(!ec);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after serialize: {}",f2.value().toIsoString()));

    BOOST_CHECK(f1.value()==f2.value());
}

BOOST_AUTO_TEST_CASE(DatetimeFieldLocal)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    f1.mutableValue()->loadCurrentLocal();
    BOOST_TEST_MESSAGE(fmt::format("before serialize: {}",f1.value().toIsoString()));

    WireBufSolid buf;
    Error ec;
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf,ec);
    BOOST_REQUIRE(!ec);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after deserialize: {}",f2.value().toIsoString()));

    BOOST_CHECK(f1.value()==f2.value());
}

BOOST_AUTO_TEST_CASE(DatetimeFieldJson)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    common::DateTime dt1{common::Date{2024,3,27},common::Time{15,21,37}};
    f1.set(dt1);
    BOOST_TEST_MESSAGE(fmt::format("before json serialize: {}",f1.value().toIsoString()));

    auto j1=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("json: \n{}\n",j1));

    u1::type o2;
    auto ok=o2.loadFromJSON(j1);
    BOOST_REQUIRE(ok);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after json serialize: {}",f2.value().toIsoString()));

    BOOST_CHECK(f1.value()==f2.value());
    BOOST_CHECK(f2.value()==dt1);
}

BOOST_AUTO_TEST_CASE(DatetimeFieldMidnight)
{
    // 2026-10-20 00:00:00.000 UTC, sent by servers for a start of the day
    auto midnightMs=common::DateTime{common::Date{2026,10,20},common::Time::makeInRange(0,0,0,1)}.toEpochMs()-1;
    auto dt1=common::DateTime::fromEpochMs(midnightMs);
    BOOST_REQUIRE(dt1.isValid());

    u1::type o1;
    auto& f1=o1.field(u1::f1);
    f1.set(dt1);

    WireBufSolid buf;
    Error ec;
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    auto ok=io::deserialize(o2,buf,ec);
    BOOST_REQUIRE(ok);
    BOOST_REQUIRE(!ec);
    const auto& f2=o2.field(u1::f1);
    BOOST_CHECK(f2.isSet());
    BOOST_CHECK(f2.value().isValid());
    BOOST_CHECK(f2.value()==dt1);
    BOOST_CHECK_EQUAL(f2.value().toEpochMs(),midnightMs);

    // json
    auto j1=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("json: \n{}\n",j1));
    u1::type o3;
    BOOST_REQUIRE(o3.loadFromJSON(j1));
    BOOST_CHECK(o3.field(u1::f1).value()==dt1);
}

BOOST_AUTO_TEST_CASE(DatetimeFieldOutOfRangeDoesNotThrow)
{
    // a packed value far beyond the supported range must give parse error but not exception
    u1::type o1;
    auto& f1=o1.field(u1::f1);

    WireBufSolid buf;
    Error ec;
    f1.set(common::DateTime::currentUtc());
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    // corrupt the payload: set all bits of the varint
    auto* data=buf.mainContainer();
    BOOST_REQUIRE(data->size()>2);
    for (size_t i=1;i<data->size();i++)
    {
        data->data()[i]=static_cast<char>(0xFF);
    }
    data->data()[data->size()-1]=static_cast<char>(0x01);

    u1::type o2;
    bool ok=true;
    BOOST_CHECK_NO_THROW(ok=io::deserialize(o2,buf,ec));
    std::ignore=ok;
}

BOOST_AUTO_TEST_SUITE_END()
