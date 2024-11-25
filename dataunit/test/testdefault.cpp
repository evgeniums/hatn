#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;
using namespace hatn::dataunit::meta;

namespace {

HDU_V2_UNIT(def1,
    HDU_V2_DEFAULT_FIELD(f30,TYPE_INT32,30,10)
    HDU_V2_ENUM(e1,One,Two,Three)
    HDU_V2_DEFAULT_FIELD(f90,HDU_V2_TYPE_ENUM(e1),90,e1::Three)
)

HDU_V2_DEFAULT_PREPARE(f90,def1::e1,def1::e1::Three)

HDU_V2_UNIT(s1,
    HDU_V2_FIELD(f1,TYPE_STRING,1,false,"Hello world!")
    HDU_V2_FIELD(f2,HDU_V2_TYPE_FIXED_STRING(32),2,false,"How are you?")
)

HDU_V2_DEFAULT_PREPARE(f1,TYPE_STRING,"Hello world!")

HDU_V2_UNIT(rs1,
    HDU_V2_FIELD(scalar1,TYPE_UINT32,1,false,10)
    HDU_V2_FIELD(scalar5,TYPE_UINT32,5)
    HDU_V2_REPEATED_FIELD(repeated2,TYPE_UINT32,2,false,10)
    HDU_V2_FIELD(subunit3,s1::TYPE,3)
    HDU_V2_FIELD(bytes4,TYPE_BYTES,4)
)

}

BOOST_AUTO_TEST_SUITE(TestDefault)

BOOST_AUTO_TEST_CASE(DefaultEnumV2)
{
    default_f90 d1;
    static_assert(d1.value==def1::e1::Three,"");
    static_assert(std::is_same<default_f90::type,def1::e1>::value,"");

    using et=HDU_V2_TYPE_ENUM(def1::e1);
    static_assert(et::isEnum::value && std::is_constructible<et::Enum, default_f90::type>::value,"");

    using dt=default_type<et,default_f90>;
    static_assert(dt::HasDefV::value,"");

    BOOST_CHECK(def1::e1::Three==d1.value);
}

BOOST_AUTO_TEST_CASE(DefaultFieldV2)
{
    def1::type v1;
    const auto& fields=def1::fields;

    auto& f30=v1.field(fields.f30);
    BOOST_CHECK(!f30.isSet());
    BOOST_CHECK_EQUAL(10,f30.value());

    auto& f90=v1.field(fields.f90);
    BOOST_CHECK(!f90.isSet());
    BOOST_CHECK(def1::e1::Three==f90.value());
}

BOOST_AUTO_TEST_CASE(DefaultStringV2)
{
    default_f1 d1;
    static_assert(std::is_same<default_f1::type,const char*>::value,"");
    BOOST_CHECK_EQUAL("Hello world!",d1.value);

    s1::type v1;
    const auto& fields1=s1::fields;

    auto& f1=v1.field(fields1.f1);
    BOOST_CHECK(!f1.isSet());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),std::string(f1.value()));
    f1.set("Hi!");
    BOOST_CHECK(f1.isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(f1.value()));
    f1.clear();
    BOOST_CHECK(f1.isSet());
    BOOST_CHECK(f1.value().empty());
    f1.set("Hi!");
    BOOST_CHECK(f1.isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(f1.value()));
    f1.reset();
    BOOST_CHECK(!f1.isSet());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),std::string(f1.value()));

    auto& f2=v1.field(fields1.f2);
    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK_EQUAL(std::string("How are you?"),std::string(f2.value()));
    f2.set("Hi!");
    BOOST_CHECK(f2.isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(f2.value()));
    f2.clear();
    BOOST_CHECK(f2.isSet());
    BOOST_CHECK(f2.value().empty());
    f2.set("Hi!");
    BOOST_CHECK(f2.isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(f2.value()));
    f2.reset();
    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK_EQUAL(std::string("How are you?"),std::string(f2.value()));
}

BOOST_AUTO_TEST_CASE(ResetClearV2)
{
    rs1::type v1;
    const auto& fields1=rs1::fields;

    auto& scalar1=v1.field(fields1.scalar1);
    BOOST_CHECK(!scalar1.isSet());
    BOOST_CHECK_EQUAL(10,scalar1.value());
    scalar1.set(100);
    BOOST_CHECK(scalar1.isSet());
    BOOST_CHECK_EQUAL(100,scalar1.value());
    scalar1.clear();
    BOOST_CHECK(scalar1.isSet());
    BOOST_CHECK_EQUAL(0,scalar1.value());
    scalar1.reset();
    BOOST_CHECK(!scalar1.isSet());
    BOOST_CHECK_EQUAL(10,scalar1.value());

    auto& scalar5=v1.field(fields1.scalar5);
    BOOST_CHECK(!scalar5.isSet());
    BOOST_CHECK_EQUAL(0,scalar5.value());
    scalar5.set(100);
    BOOST_CHECK(scalar5.isSet());
    BOOST_CHECK_EQUAL(100,scalar5.value());
    scalar5.clear();
    BOOST_CHECK(scalar5.isSet());
    BOOST_CHECK_EQUAL(0,scalar5.value());
    scalar5.reset();
    BOOST_CHECK(!scalar5.isSet());
    BOOST_CHECK_EQUAL(0,scalar5.value());

    auto& repeated2=v1.field(fields1.repeated2);
    BOOST_CHECK(!repeated2.isSet());
    BOOST_CHECK_EQUAL(0,repeated2.count());
    repeated2.appendValues(3);
    BOOST_CHECK(repeated2.isSet());
    BOOST_CHECK_EQUAL(3,repeated2.count());
    BOOST_CHECK_EQUAL(10,repeated2.at(0));
    BOOST_CHECK_EQUAL(10,repeated2.at(1));
    BOOST_CHECK_EQUAL(10,repeated2.at(2));
    repeated2.clear();
    BOOST_CHECK(repeated2.isSet());
    BOOST_CHECK_EQUAL(0,repeated2.count());
    repeated2.appendValues(3);
    BOOST_CHECK(repeated2.isSet());
    BOOST_CHECK_EQUAL(3,repeated2.count());
    BOOST_CHECK_EQUAL(10,repeated2.value().at(0));
    BOOST_CHECK_EQUAL(10,repeated2.value().at(1));
    BOOST_CHECK_EQUAL(10,repeated2.value().at(2));
    repeated2.reset();
    BOOST_CHECK(!repeated2.isSet());
    BOOST_CHECK_EQUAL(0,repeated2.count());

    auto& subunit3=v1.field(fields1.subunit3);
    BOOST_CHECK(!subunit3.isSet());
    BOOST_CHECK(!subunit3.field(s1::f1).isSet());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),std::string(subunit3.field(s1::f1).value()));
    subunit3.mutableValue()->field(s1::f1).set("Hi!");
    BOOST_CHECK(subunit3.isSet());
    BOOST_CHECK(subunit3.field(s1::f1).isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(subunit3.field(s1::f1).value()));
    subunit3.clear();
    BOOST_CHECK(subunit3.isSet());
    BOOST_CHECK(subunit3.field(s1::f1).isSet());
    BOOST_CHECK(subunit3.field(s1::f1).value().empty());
    subunit3.mutableValue()->field(s1::f1).set("Hi!");
    BOOST_CHECK(subunit3.isSet());
    BOOST_CHECK(subunit3.field(s1::f1).isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(subunit3.field(s1::f1).value()));
    subunit3.reset();
    BOOST_CHECK(!subunit3.isSet());
    BOOST_CHECK(!subunit3.field(s1::f1).isSet());
    BOOST_CHECK_EQUAL(std::string("Hello world!"),std::string(subunit3.field(s1::f1).value()));

    auto& bytes4=v1.field(fields1.bytes4);
    BOOST_CHECK(!bytes4.isSet());
    BOOST_CHECK_EQUAL(0,bytes4.value().size());
    bytes4.set("Hi!");
    BOOST_CHECK(bytes4.isSet());
    BOOST_CHECK_EQUAL(std::string("Hi!"),std::string(bytes4.value()));
    bytes4.clear();
    BOOST_CHECK(bytes4.isSet());
    BOOST_CHECK_EQUAL(0,bytes4.value().size());
    bytes4.reset();
    BOOST_CHECK(!bytes4.isSet());
    BOOST_CHECK_EQUAL(0,bytes4.value().size());
}

BOOST_AUTO_TEST_SUITE_END()
