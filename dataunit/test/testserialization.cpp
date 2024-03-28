#include <boost/test/unit_test.hpp>

#define HDU_DATAUNIT_EXPORT

#include <hatn/common/pmr/withstaticallocator.h>
#define HATN_WITH_STATIC_ALLOCATOR_SRC
#ifdef HATN_WITH_STATIC_ALLOCATOR_SRC
#include <hatn/common/pmr/withstaticallocator.ipp>
#define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC
#else
#define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_H
#endif

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>
#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/visitors/serialize.h>

namespace {

HDU_DATAUNIT_EMPTY(du0)
HDU_INSTANTIATE_DATAUNIT(du0)

HDU_DATAUNIT(du1,
  HDU_FIELD(field2,TYPE_INT32,2)
)
HDU_INSTANTIATE_DATAUNIT(du1)

HDU_DATAUNIT(du2,
    HDU_FIELD_REQUIRED(field3,TYPE_INT32,10)
)
HDU_INSTANTIATE_DATAUNIT(du2)

HDU_DATAUNIT(du3,
    HDU_FIELD_REQUIRED(field4,TYPE_STRING,30)
)
HDU_INSTANTIATE_DATAUNIT(du3)

HDU_DATAUNIT(du4,
    HDU_FIELD_DATAUNIT(f2,du2::TYPE,2)
    HDU_FIELD_DATAUNIT(f3,du3::TYPE,3)
)
HDU_INSTANTIATE_DATAUNIT(du4)

} // anonymous namespace

namespace du=HATN_DATAUNIT_NAMESPACE;

BOOST_AUTO_TEST_SUITE(TestSerialization)

BOOST_AUTO_TEST_CASE(SerializeEmptyUnit)
{
    using traits=du0::traits;
    using type=traits::type;

    type obj1;
    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(!r);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(SerializeIntField)
{
    using traits=du1::traits;
    using type=traits::type;

    type obj1;
    auto& f1=obj1.field(du1::field2);
    f1.set(300);
    BOOST_CHECK_EQUAL(300,f1.get());

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(!r);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL(300,obj2.field(du1::field2).get());
}

BOOST_AUTO_TEST_CASE(SerializeStringField)
{
    using traits=du3::traits;
    using type=traits::type;

    type obj1;
    auto& f1=obj1.field(du3::field4);
    f1.set("Hello world!");
    BOOST_CHECK_EQUAL("Hello world!",f1.c_str());

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(!r);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj2.field(du3::field4).c_str());
}

BOOST_AUTO_TEST_CASE(SerializeSubunitField)
{
    using traits=du4::traits;
    using type=traits::type;

    type obj1;
    auto f3=obj1.field(du4::f3).mutableValue();
    auto& f3_4=f3->field(du3::field4);
    f3_4.set("Hello world!");
    BOOST_CHECK_EQUAL("Hello world!",f3_4.c_str());

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(!r);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_REQUIRE(ok);

    const auto& c_f3=obj2.field(du4::f3).get();
    const auto& c_f3_4=c_f3.field(du3::field4);
    BOOST_CHECK_EQUAL("Hello world!",c_f3_4.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
