#include <boost/test/unit_test.hpp>

#include <hatn/common/meta/compilecounter.h>
#include <hatn/common/meta/interfacespack.h>

HATN_USING
HATN_COMMON_USING

namespace {

struct Interface1
{
    uint32_t a;
    uint32_t b;
};

struct Interface2
{
    char c[256];
};

using AAA=VInterfacesPack<Interface1,Interface2>;

HATN_COUNTER_MAKE(c1)
constexpr auto v0=HATN_COUNTER_GET(c1);
static_assert(0==v0,"");
HATN_COUNTER_INC(c1)
constexpr auto v1=HATN_COUNTER_GET(c1);
static_assert(1==v1,"");

}

BOOST_AUTO_TEST_SUITE(TestMetaUtils)

BOOST_AUTO_TEST_CASE(InterfacesPack)
{
    AAA a;
    int checkSize=sizeof(Interface1)+sizeof(Interface2);
    int size1=sizeof(a);
    BOOST_CHECK_EQUAL(size1,checkSize);

    auto& i1=a.getInterface<Interface1>();
    BOOST_CHECK_EQUAL(sizeof(i1),sizeof(Interface1));
    auto& i2=a.getInterface<Interface2>();
    BOOST_CHECK_EQUAL(sizeof(i2),sizeof(Interface2));
}

BOOST_AUTO_TEST_SUITE_END()
