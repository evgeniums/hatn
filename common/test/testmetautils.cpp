#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#include <hatn/common/error.h>

#include <hatn/common/meta/compilecounter.h>
#include <hatn/common/meta/interfacespack.h>
#include <hatn/common/meta/hasmethod.h>
#include <hatn/common/meta/chain.h>

namespace hana=boost::hana;

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

struct WithGet
{
    int get() const
    {
        return 0;
    }
};

struct WithSet
{
    void set(int val)
    {
        m_val=val;
    }

    int m_val=0;
};

struct WithSetGet : public WithSet
{
    int get() const
    {
        return m_val;
    }
};

struct WithSetGetCtor : public WithSetGet
{
    WithSetGetCtor(int val)
    {
        m_val=val;
    }
};

HATN_PREPARE_HAS_METHOD(set)
HATN_PREPARE_HAS_METHOD(get)

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

BOOST_AUTO_TEST_CASE(HasMethod)
{
    auto hasGet1_1=has_get<WithGet>();
    auto hasGet1_2=has_get<WithSet>();
    auto hasGet1_3=has_get<WithSetGet>();
    auto hasGet1_4=has_get<WithSetGetCtor>();
    static_assert(decltype(hasGet1_1)::value,"");
    static_assert(!decltype(hasGet1_2)::value,"");
    static_assert(decltype(hasGet1_3)::value,"");
    static_assert(decltype(hasGet1_4)::value,"");

    auto hasSet1_1=has_set<WithGet>(1);
    auto hasSet1_2=has_set<WithSet>(1);
    auto hasSet1_3=has_set<WithSetGet>(1);
    auto hasSet1_4=has_set<WithSetGetCtor>(1);
    static_assert(!decltype(hasSet1_1)::value,"");
    static_assert(decltype(hasSet1_2)::value,"");
    static_assert(decltype(hasSet1_3)::value,"");
    static_assert(decltype(hasSet1_4)::value,"");

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(Chain)
{
    size_t totalCount=0;

    auto node1=chain::node(
        [&totalCount](auto* next, const Error& ec, size_t count)
        {
            totalCount+=count;
            BOOST_CHECK_EQUAL(count,1);
            BOOST_TEST_MESSAGE(fmt::format("In node 1, count {}, next {}",count,fmt::ptr(next)));
            if (next!=nullptr)
            {
                (*next)(ec,count+1,100);
            }
        }
        );

    auto node2=chain::node(
        [&totalCount](auto* next, const Error& ec, size_t count, int val)
        {
            totalCount+=count;
            BOOST_CHECK_EQUAL(count,2);
            BOOST_CHECK_EQUAL(val,100);
            BOOST_TEST_MESSAGE(fmt::format("In node 2, count {}, val {}, next {}",count,val,fmt::ptr(next)));
            if (next!=nullptr)
            {
                (*next)(ec,count+1);
            }
        }
        );

    auto lastNode=chain::node(
        [&totalCount](const Error& ec, size_t count)
        {
            totalCount+=count;
            BOOST_CHECK_EQUAL(count,3);
            BOOST_TEST_MESSAGE(fmt::format("In last node, count {}",count));
        }
        );

    auto nodes=chain::nodes(node1,node2,lastNode);
    auto chain=chain::link(nodes);
    chain(Error{1},1);

    BOOST_CHECK_EQUAL(totalCount,6);
}

BOOST_AUTO_TEST_SUITE_END()
