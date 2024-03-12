#include <iostream>
#include <boost/test/unit_test.hpp>

#include <hatn/common/environment.h>
#include <hatn/test/interfacetestdll.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestEnvironment)

struct OpInterface0 : public HATN_COMMON_NAMESPACE::Interface<OpInterface0>
{
    int a=1;
    char b=2;
    int c=3;
};
struct OpInterface1 : public HATN_COMMON_NAMESPACE::Interface<OpInterface1>
{
    int i=7;
    uint16_t j=6;
    char k=5;
    int64_t e=4;
};
struct OpInterface2 : public HATN_COMMON_NAMESPACE::Interface<OpInterface2>
{
    int i=7;
    uint16_t j=6;
    char k=5;
    int64_t e=4;
    float q;
};
struct OpInterface3 : public HATN_COMMON_NAMESPACE::Interface<OpInterface3>
{
    HATN_CUID_DECLARE()

    int a=1;
    char b=2;
    int c=3;
};
HATN_CUID_INIT(OpInterface3)

struct OpInterface4 : public HATN_COMMON_NAMESPACE::MultiInterface<OpInterface4,OpInterface2,OpInterface0>
{
};

struct OpInterface7 : public OpInterface0
{
};

struct OpInterface8 : public OpInterface7, public OpInterface1
{
};

struct OpInterface5 : public OpInterface8, public OpInterface2, public HATN_COMMON_NAMESPACE::ClassUid<OpInterface5,OpInterface2,OpInterface0,OpInterface1>
{
    using HATN_COMMON_NAMESPACE::ClassUid<OpInterface5,OpInterface2,OpInterface0,OpInterface1>::cuid;
    using HATN_COMMON_NAMESPACE::ClassUid<OpInterface5,OpInterface2,OpInterface0,OpInterface1>::cuids;
};

struct Env1 : public EnvironmentPack<OpInterface0,OpInterface1,OpInterface2,OpInterface3>
{
    //! Constructor
    Env1()
    {}
};

struct Env2 : public EnvironmentPack<OpInterface4,OpInterface1>
{
    //! Constructor
    Env2()
    {}
};

struct Env3 : public EnvironmentPack<OpInterface5,OpInterface1>
{
    //! Constructor
    Env3()
    {}
};

BOOST_AUTO_TEST_CASE(EnvInterfaces)
{
#ifdef TEST_INTERFACE_DLL
    HATN_COMMON_NAMESPACE::Johny jonny;
#ifdef TEST_INTERFACE_DLL_PRINT
    std::cerr<<"Test cuids: ";
    for (auto&& it: HATN_COMMON_NAMESPACE::Johny::cuids())
    {
        std::cerr<<"0x"<<std::hex<<it<<",";
    }
    std::cerr<<std::endl;
#endif
    BOOST_CHECK_EQUAL(jonny.id1,HATN_COMMON_NAMESPACE::Johny1::cuid());
    BOOST_CHECK_EQUAL(jonny.id2,HATN_COMMON_NAMESPACE::Johny2::cuid());
    BOOST_CHECK_EQUAL(jonny.id3,HATN_COMMON_NAMESPACE::Johny3::cuid());
    BOOST_CHECK_EQUAL(jonny.id4,HATN_COMMON_NAMESPACE::Johny::cuid());
    BOOST_CHECK(HATN_COMMON_NAMESPACE::Johny::cuids()==jonny.ids);

#endif

    Env1 env;
    Env1 env2;

    auto* interface22=env2.interface<OpInterface2>();
    auto* interface22Pos=reinterpret_cast<OpInterface2*>(env2.interfaceAtPos(2));
    auto* interface22Map=env2.interfaceMapped<OpInterface2>();
    BOOST_CHECK_EQUAL(interface22,interface22Pos);
    BOOST_CHECK_EQUAL(interface22,interface22Map);

    auto* interface21=env2.interface<OpInterface1>();
    auto* interface21Pos=reinterpret_cast<OpInterface1*>(env2.interfaceAtPos(1));
    auto* interface21Map=env2.interfaceMapped<OpInterface1>();
    BOOST_CHECK_EQUAL(interface21,interface21Pos);
    BOOST_CHECK_EQUAL(interface21,interface21Map);

    auto* interface2=env.interface<OpInterface2>();
    auto* interface2Pos=reinterpret_cast<OpInterface2*>(env.interfaceAtPos(2));
    BOOST_CHECK_EQUAL(interface2,interface2Pos);

    auto* interface0=env.interface<OpInterface0>();
    BOOST_CHECK_EQUAL(interface0->a,1);
    BOOST_CHECK_EQUAL(interface0->b,2);
    BOOST_CHECK_EQUAL(interface0->c,3);
    auto* interface0Pos=reinterpret_cast<OpInterface0*>(env.interfaceAtPos(0));
    auto* interface0Map=env.interfaceMapped<OpInterface0>();
    BOOST_CHECK_EQUAL(interface0,interface0Pos);
    BOOST_CHECK_EQUAL(interface0,interface0Map);

    auto* interface1=env.interface<OpInterface1>();
    BOOST_CHECK_EQUAL(interface1->i,7);
    BOOST_CHECK_EQUAL(interface1->j,6);
    BOOST_CHECK_EQUAL(interface1->k,5);
    BOOST_CHECK_EQUAL(interface1->e,4);
    auto* interface1Pos=reinterpret_cast<OpInterface1*>(env.interfaceAtPos(1));
    BOOST_CHECK_EQUAL(interface1,interface1Pos);

    auto* interface3=env.interface<OpInterface3>();
    auto* interface3Pos=reinterpret_cast<OpInterface3*>(env.interfaceAtPos(3));
    BOOST_CHECK_EQUAL(interface3,interface3Pos);
}

BOOST_AUTO_TEST_SUITE_END()
