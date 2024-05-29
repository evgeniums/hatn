#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestDateTime)

BOOST_AUTO_TEST_CASE(TestDate)
{
    Date dt1;
    BOOST_REQUIRE(dt1.isNull());
    BOOST_REQUIRE(!dt1.isValid());

    auto dt2=Date::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("today: {:04d}{:02d}{:02d}",dt2.year(),dt2.month(),dt2.day()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    auto dt3=Date{2024,03,31};
    BOOST_TEST_MESSAGE(fmt::format("fixed date: {:04d}{:02d}{:02d}",dt3.year(),dt3.month(),dt3.day()));
    BOOST_REQUIRE(!dt3.isNull());
    BOOST_REQUIRE(dt3.isValid());
    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),03);
    BOOST_CHECK_EQUAL(dt3.day(),31);

    auto ec=dt3.setYear(2025);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.year(),2025);
    ec=dt3.setMonth(12);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.month(),12);
    ec=dt3.setDay(15);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.day(),15);
    ec=dt3.set(20240205);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),02);
    BOOST_CHECK_EQUAL(dt3.day(),05);

    ec=dt3.setYear(0);
    BOOST_CHECK(ec);
    ec=dt3.setMonth(15);
    BOOST_CHECK(ec);
    ec=dt3.setMonth(0);
    BOOST_CHECK(ec);
    ec=dt3.setDay(32);
    BOOST_CHECK(ec);
    ec=dt3.setDay(0);
    BOOST_CHECK(ec);
    ec=dt3.set(0);
    BOOST_CHECK(ec);
    ec=dt3.set(100);
    BOOST_CHECK(ec);
    ec=dt3.set(20351555);
    BOOST_CHECK(ec);

    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),02);
    BOOST_CHECK_EQUAL(dt3.day(),05);

    auto h1=[]{
        auto dt4=Date{2024,03,33};
        std::ignore=dt4;
    };
    BOOST_CHECK_THROW(h1(),std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
