#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestDateTime)

BOOST_AUTO_TEST_CASE(TestDate)
{
    Date dt;
    BOOST_REQUIRE(dt.isNull());
    BOOST_REQUIRE(!dt.isValid());
}

BOOST_AUTO_TEST_SUITE_END()
