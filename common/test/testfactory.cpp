#include <boost/test/unit_test.hpp>

#include <hatn/common/classuid.h>
#include <hatn/common/factory.h>

HATN_USING
HATN_COMMON_USING

namespace
{
struct Stub : public ClassUid<Stub>
{
    uint32_t id=100;
};

struct BaseT1 : public ClassUid<BaseT1>
{
    constexpr static const uint32_t ID=3;

    virtual ~BaseT1()=default;
    BaseT1(const BaseT1&)=default;
    BaseT1(BaseT1&&) =default;
    BaseT1& operator=(const BaseT1&)=default;
    BaseT1& operator=(BaseT1&&) =default;

    uint32_t id;

    BaseT1(uint32_t id=ID):id(id)
    {}

    virtual uint32_t index() const
    {
        return id;
    }
};
struct DerivedT1 : public BaseT1, public ClassUid<DerivedT1>
{
    using ClassUid<DerivedT1>::cuid;

    using BaseT1::BaseT1;
    virtual uint32_t index() const override
    {
        return id+1u;
    }
};
struct DerivedT2 : public DerivedT1, public ClassUid<DerivedT2>
{
    using ClassUid<DerivedT2>::cuid;
    using DerivedT1::DerivedT1;
    virtual uint32_t index() const override
    {
        return id+2u;
    }
};

struct Class2 : public ClassUid<Class2>
{
    size_t id;
    std::string str;

    Class2(size_t id, std::string str):id(id),str(std::move(str))
    {}

    size_t index() const
    {
        return id+10u;
    }
    std::string message() const
    {
        return str;
    }
};

struct Class3 : public ClassUid<Class3>
{
    std::string str;

    Class3(std::string str):str(std::move(str))
    {}

    std::string message() const
    {
        return str;
    }
};

using Factory1=MakeFactory<BuilderMeta<BuilderTraits<>::Build<DerivedT1>,BaseT1,DerivedT1>>;
using Factory2=MakeFactory<BuilderMeta<BuilderTraits<uint32_t>::Build<DerivedT1>,BaseT1,DerivedT1>>;
using Factory3=MakeFactory<BuilderMeta<BuilderTraits<uint32_t>::Build<DerivedT1>,BaseT1>>;
using Factory4=MakeFactory<BuilderMeta<BuilderTraits<uint32_t>::Build<DerivedT2>,BaseT1,DerivedT2>>;

using Factory5=MakeFactory<
                    BuilderMeta<BuilderTraits<size_t,std::string>::Build<Class2>,Class2>,
                    BuilderMeta<BuilderTraits<uint32_t>::Build<DerivedT2>,BaseT1,DerivedT2>
                >;
}

BOOST_AUTO_TEST_SUITE(TestFactory)

BOOST_AUTO_TEST_CASE(SingleBuilderNoArgs)
{
    Factory1 factory;
    auto baseObj1=factory.create<BaseT1>();
    BOOST_REQUIRE(baseObj1);
    BOOST_CHECK_EQUAL(baseObj1->index(),(BaseT1::ID+1));
    delete baseObj1;

    auto derivedObj1=factory.create<DerivedT1>();
    BOOST_REQUIRE(derivedObj1);
    BOOST_CHECK_EQUAL(derivedObj1->index(),(BaseT1::ID+1));
    delete derivedObj1;
}

BOOST_AUTO_TEST_CASE(SingleBuilderArgs)
{
    Factory2 factory;
    auto baseObj1=factory.create<BaseT1>(10u);
    BOOST_REQUIRE(baseObj1);
    BOOST_CHECK_EQUAL(baseObj1->index(),11);
    delete baseObj1;

    auto derivedObj1=factory.create<DerivedT1>(20u);
    BOOST_REQUIRE(derivedObj1);
    BOOST_CHECK_EQUAL(derivedObj1->index(),21);
    delete derivedObj1;
}

BOOST_AUTO_TEST_CASE(SingleBuilderOnlyBase)
{
    Factory3 factory;
    auto baseObj1=factory.create<BaseT1>(10u);
    BOOST_REQUIRE(baseObj1);
    BOOST_CHECK_EQUAL(baseObj1->index(),11u);
    delete baseObj1;

    auto derivedObj1=factory.create<DerivedT1>(20u);
    BOOST_CHECK(derivedObj1==nullptr);

    auto derivedObj2=factory.create<DerivedT2>(30u);
    BOOST_CHECK(derivedObj2==nullptr);
    auto stub=factory.create<Stub>(50u);
    BOOST_CHECK(stub==nullptr);
}

BOOST_AUTO_TEST_CASE(SingleBuilderFiltered)
{
    Factory4 factory;
    auto baseObj1=factory.create<BaseT1>(10u);
    BOOST_REQUIRE(baseObj1);
    BOOST_CHECK_EQUAL(baseObj1->index(),12u);
    delete baseObj1;

    auto derivedObj1=factory.create<DerivedT1>(20u);
    BOOST_CHECK(derivedObj1==nullptr);

    auto derivedObj2=factory.create<DerivedT2>(30u);
    BOOST_REQUIRE(derivedObj2);
    BOOST_CHECK_EQUAL(derivedObj2->index(),32u);
    delete derivedObj2;
}

BOOST_AUTO_TEST_CASE(MultipleBuilder)
{
    Factory5 factory;
    auto baseObj1=factory.create<BaseT1>(10u);
    BOOST_REQUIRE(baseObj1);
    BOOST_CHECK_EQUAL(baseObj1->index(),12u);
    delete baseObj1;

    auto derivedObj1=factory.create<DerivedT1>(20u);
    BOOST_CHECK(derivedObj1==nullptr);

    auto derivedObj2=factory.create<DerivedT2>(30u);
    BOOST_REQUIRE(derivedObj2);
    BOOST_CHECK_EQUAL(derivedObj2->index(),32u);
    delete derivedObj2;

    auto class2=factory.create<Class2>(static_cast<size_t>(100),std::string("Hello"));
    BOOST_REQUIRE(class2);
    BOOST_CHECK_EQUAL(class2->index(),110u);
    BOOST_CHECK_EQUAL(class2->message(),std::string("Hello"));
    delete class2;

    factory.addBuilder(
                    BuilderMeta<BuilderTraits<std::string>::Build<Class3>,Class3>::builder()
                );
    auto class3=factory.create<Class3>(std::string("world"));
    BOOST_REQUIRE(class3);
    BOOST_CHECK_EQUAL(class3->message(),std::string("world"));
    delete class3;
}

struct ClassNameT1 : public ClassUid<ClassNameT1>
{
    public:

        HATN_CUID_DECLARE()
};
HATN_CUID_INIT(ClassNameT1)

struct ClassNameT2 : public ClassNameT1
{
    public:

        HATN_CUID_DECLARE()
};
HATN_CUID_INIT(ClassNameT2)

struct ClassNameT3 : public ClassUid<ClassNameT3>
{
    public:

        HATN_CUID_DECLARE()
};
HATN_CUID_INIT(ClassNameT3)

struct ClassNameMulti : public MultiInterface<ClassNameMulti,ClassNameT1,ClassNameT3>
{
   public:

       HATN_CUID_DECLARE_MULTI()
};
HATN_CUID_INIT_MULTI(ClassNameMulti)

BOOST_AUTO_TEST_CASE(CheckHumanReadableType)
{
    ClassNameT1 obj1;
    auto name1=obj1.humanReadableType();
    BOOST_CHECK_EQUAL(name1,"ClassNameT1");

    ClassNameT2 obj2;
    auto name2=obj2.humanReadableType();
    BOOST_CHECK_EQUAL(name2,"ClassNameT2");

    ClassNameMulti obj3;
    auto name3=obj3.humanReadableType();
    BOOST_CHECK_EQUAL(name3,"ClassNameMulti");
}

BOOST_AUTO_TEST_SUITE_END()
