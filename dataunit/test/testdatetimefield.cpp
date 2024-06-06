#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>
#include <hatn/dataunit/visitors.h>

HATN_USING
HATN_DATAUNIT_USING
using namespace HATN_DATAUNIT_NAMESPACE::types;
using namespace HATN_DATAUNIT_NAMESPACE::meta;

namespace {

struct f0_name
{
    constexpr static const char* name="f0";
};

HDU_UNIT(u1,
    HDU_FIELD(f1,TYPE_DATETIME,1)
)

}

BOOST_AUTO_TEST_SUITE(TestDatetimeField)

BOOST_AUTO_TEST_CASE(DatetimeFieldUtc)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    f1.get().toCurrentUtc();
    f1.markSet();
    BOOST_TEST_MESSAGE(fmt::format("before serialize: {}",f1.get().toIsoString()));

    WireBufSolid buf;
    Error ec;
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf,ec);
    BOOST_REQUIRE(!ec);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after serialize: {}",f2.get().toIsoString()));

    BOOST_CHECK(f1.get()==f2.get());
}

BOOST_AUTO_TEST_CASE(DatetimeFieldLocal)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    f1.get().toCurrentLocal();
    f1.markSet();
    BOOST_TEST_MESSAGE(fmt::format("before serialize: {}",f1.get().toIsoString()));

    WireBufSolid buf;
    Error ec;
    io::serialize(o1,buf,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf,ec);
    BOOST_REQUIRE(!ec);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after deserialize: {}",f2.get().toIsoString()));

    BOOST_CHECK(f1.get()==f2.get());
}

BOOST_AUTO_TEST_CASE(DatetimeFieldJson)
{
    u1::type o1;
    auto& f1=o1.field(u1::f1);
    common::DateTime dt1{common::Date{2024,3,27},common::Time{15,21,37}};
    f1.set(dt1);
    BOOST_TEST_MESSAGE(fmt::format("before json serialize: {}",f1.get().toIsoString()));

    auto j1=o1.toString(true);
    BOOST_TEST_MESSAGE(fmt::format("json: \n{}\n",j1));

    u1::type o2;
    auto ok=o2.loadFromJSON(j1);
    BOOST_REQUIRE(ok);
    const auto& f2=o2.field(u1::f1);
    BOOST_TEST_MESSAGE(fmt::format("after json serialize: {}",f2.get().toIsoString()));

    BOOST_CHECK(f1.get()==f2.get());
    BOOST_CHECK(f2.get()==dt1);
}

BOOST_AUTO_TEST_SUITE_END()
