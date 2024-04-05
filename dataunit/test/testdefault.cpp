#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/unitmacros.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

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

HDU_V2_UNIT(b1,
    HDU_V2_FIELD(f1,TYPE_BOOL,1)
)

HDU_V2_UNIT_WITH(ext1,(HDU_V2_BASE(b1)),
    HDU_V2_FIELD(f2,TYPE_BOOL,2)
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

BOOST_AUTO_TEST_CASE(DefaultExtBoolV2)
{
    b1::type v1;
    const auto& fields1=b1::fields;

    auto& f1=v1.field(fields1.f1);
    BOOST_CHECK(!f1.isSet());
    BOOST_CHECK(!f1.value());

    ext1::type v2;
    const auto& fields2=ext1::fields;

    auto& f2=v2.field(fields2.f2);
    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK(!f2.value());

    auto& f2_1=v2.field(fields1.f1);
    BOOST_CHECK(!f2_1.isSet());
    BOOST_CHECK(!f2_1.value());

    f2_1.set(true);
    BOOST_CHECK(f2_1.isSet());
    BOOST_CHECK(f2_1.value());

    BOOST_CHECK(!f2.isSet());
    BOOST_CHECK(!f2.value());
}

BOOST_AUTO_TEST_SUITE_END()
