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

BOOST_AUTO_TEST_SUITE_END()
