#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>
#include <hatn/dataunit/tags.h>

using namespace hatn::dataunit;
using namespace hatn::dataunit::types;
using namespace hatn::dataunit::meta;

namespace {

HDU_TAG_DECLARE(description)
HDU_TAG_DECLARE(longname)
HDU_TAG_DECLARE(index)

HDU_UNIT(u1,
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(f2,TYPE_DOUBLE,2)
)

}

BOOST_AUTO_TEST_SUITE(TestTags)

BOOST_AUTO_TEST_CASE(TagsConstruction)
{
    BOOST_CHECK_EQUAL(HDU_TAG_NAME(description),"description");
    BOOST_CHECK_EQUAL(HDU_TAG_NAME(longname),"longname");
    BOOST_CHECK_EQUAL(HDU_TAG_NAME(index),"index");

    auto t1=HDU_TAG(description,"Field 1 unsigned int 32");
    BOOST_CHECK_EQUAL(HDU_TAG_VALUE(t1),"Field 1 unsigned int 32");
    BOOST_CHECK_EQUAL(HDU_TAG_KEY_NAME(t1),"description");

    auto ft1=field_tags(u1::f1,t1);
    BOOST_CHECK_EQUAL(HDU_EXTRACT_FIELD_TAG(hana::second(ft1),description),"Field 1 unsigned int 32");

    auto ut1=unit_field_tags(ft1,field_tags(u1::f2,HDU_TAG(description,"Field 2 double")));

    const auto& ft2=hana::at_key(ut1,hana::type_c<std::decay_t<decltype(u1::f2)>>);
    const auto& ftu1=hana::at_key(ut1,hana::type_c<std::decay_t<decltype(u1::f1)>>);

    auto&& ftu1Descr=HDU_EXTRACT_FIELD_TAG(ftu1,description);
    auto&& ft2Descr=HDU_EXTRACT_FIELD_TAG(ft2,description);
    BOOST_CHECK_EQUAL(ft2Descr,"Field 2 double");
    BOOST_CHECK_EQUAL(ftu1Descr,"Field 1 unsigned int 32");

    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut1,u1::f2,description),"Field 2 double");
    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut1,u1::f1,description),"Field 1 unsigned int 32");

    auto ut2=unit_field_tags(field_tags(u1::f1,HDU_TAG(longname,"field1"),HDU_TAG(description,"Field 1 unsigned int 32"),HDU_TAG(index,1)),
                             field_tags(u1::f2,HDU_TAG(longname,"field2"),HDU_TAG(description,"Field 2 double"),HDU_TAG(index,2))
                            );

    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f1,longname),"field1");
    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f1,description),"Field 1 unsigned int 32");
    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f1,index),1);

    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f2,longname),"field2");
    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f2,description),"Field 2 double");
    BOOST_CHECK_EQUAL(HDU_FIELD_TAG(ut2,u1::f2,index),2);
}

BOOST_AUTO_TEST_SUITE_END()
