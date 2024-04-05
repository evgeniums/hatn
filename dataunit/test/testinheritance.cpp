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

BOOST_AUTO_TEST_SUITE(TestInherit)

BOOST_AUTO_TEST_CASE(InheritanceV2)
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
