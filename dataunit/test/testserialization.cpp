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
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/detail/wirebuf.ipp>

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

HDU_DATAUNIT(du5,
    HDU_FIELD_REPEATED(field5,TYPE_STRING,50)
)
HDU_INSTANTIATE_DATAUNIT(du5)

HDU_DATAUNIT(du6,
    HDU_FIELD_REQUIRED(field1,TYPE_INT32,1)
)
HDU_INSTANTIATE_DATAUNIT(du6)

HDU_DATAUNIT(du7,
    HDU_FIELD_DATAUNIT(f2,du6::TYPE,2)
)
HDU_INSTANTIATE_DATAUNIT(du7)

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
    BOOST_CHECK(r==0);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);

    du::WireBufSolid buf2;
    r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r==0);

    auto size=du::io::size(obj1);
    BOOST_CHECK_EQUAL(0,size);
}

BOOST_AUTO_TEST_CASE(SerializeIntField)
{
    using traits=du1::traits;
    using type=traits::type;

    type obj1;
    auto& f1=obj1.field(du1::field2);
    f1.set(300);
    BOOST_CHECK_EQUAL(300,f1.get());
    auto size=du::io::size(obj1);
    BOOST_CHECK_EQUAL(8,size);

    auto du1f2=du1::field2;
    const auto* f1Parser=obj1.fieldParser<du::WireDataSingle>(du1f2);
    BOOST_CHECK(static_cast<bool>(f1Parser));

    const auto* noParser=obj1.fieldParser<du::WireDataSingle>(100);
    BOOST_CHECK(!static_cast<bool>(noParser));

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(r>0);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL(300,obj2.field(du1::field2).get());

    du::WireBufSolid buf2;
    r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL(300,obj3.field(du1::field2).get());
}

BOOST_AUTO_TEST_CASE(SerializeStringField)
{
    using traits=du3::traits;
    using type=traits::type;

    type obj1;
    auto& f1=obj1.field(du3::field4);
    f1.set("Hello world!");
    BOOST_CHECK_EQUAL("Hello world!",f1.c_str());
    auto size=du::io::size(obj1);
    BOOST_CHECK_EQUAL(20,size);

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(r>0);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj2.field(du3::field4).c_str());

    du::WireBufSolid buf2;
    r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj3.field(du3::field4).c_str());
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
    auto size=du::io::size(obj1);
    BOOST_CHECK_EQUAL(46,size);

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(r>0);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_REQUIRE(ok);

    const auto& c_f3=obj2.field(du4::f3).get();
    const auto& c_f3_4=c_f3.field(du3::field4);
    BOOST_CHECK_EQUAL("Hello world!",c_f3_4.c_str());

    du::WireBufSolid buf2;
    r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);
    const auto& c3_f3=obj3.field(du4::f3).get();
    const auto& c3_f3_4=c3_f3.field(du3::field4);
    BOOST_CHECK_EQUAL("Hello world!",c3_f3_4.c_str());
}

BOOST_AUTO_TEST_CASE(SerializeRepeatedField)
{
    using traits=du5::traits;
    using type=traits::type;

    type obj1;
    auto& f5=obj1.field(du5::field5);
    f5.addValue("Hello world!");
    BOOST_CHECK_EQUAL("Hello world!",f5.value(0).buf()->c_str());
    auto size=du::io::size(obj1);
    BOOST_CHECK_EQUAL(20,size);

    du::WireDataSingle buf1;
    auto r=du::io::serialize(obj1,buf1);
    BOOST_CHECK(r>0);
    type obj2;
    auto ok=obj2.parse(buf1);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj2.field(du5::field5).value(0).buf()->c_str());

    du::WireBufSolid buf2;
    r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj3.field(du5::field5).value(0).buf()->c_str());
}

BOOST_AUTO_TEST_CASE(SerializeSubunitFieldWithRequired)
{
    using traits=du7::traits;
    using type=traits::type;

    type obj1;
    auto f2=obj1.field(du7::f2).mutableValue();
    auto& f2_1=f2->field(du6::field1);
    BOOST_CHECK(!f2_1.isSet());
    f2_1.set(112233);
    BOOST_CHECK(f2_1.isSet());
    BOOST_CHECK_EQUAL(112233,f2_1.value());

    du::WireBufSolid buf2;
    auto r=du::io::serialize(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    auto ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_SUITE_END()
