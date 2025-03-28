#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

namespace {

HDU_V2_UNIT_EMPTY(du0)

HDU_V2_UNIT(du1,
  HDU_V2_FIELD(field2,TYPE_INT32,2)
)

HDU_V2_UNIT(du2,
    HDU_V2_REQUIRED_FIELD(field3,TYPE_INT32,10)
)

HDU_V2_UNIT(du3,
    HDU_V2_REQUIRED_FIELD(field4,TYPE_STRING,30)
)

HDU_V2_UNIT(du4,
    HDU_V2_FIELD(f2,du2::TYPE,2)
    HDU_V2_FIELD(f3,du3::TYPE,3)
)

HDU_V2_UNIT(du5,
    HDU_V2_REPEATED_FIELD(field5,TYPE_STRING,50,false,Auto,Counted)
)

HDU_V2_UNIT(du6,
    HDU_V2_REQUIRED_FIELD(field1,TYPE_INT32,1)
)

HDU_V2_UNIT(du7,
    HDU_V2_FIELD(f2,du6::TYPE,2)
)

HDU_UNIT(du8,
    HDU_FIELD(f2,TYPE_UINT32,2)
    HDU_FIELD(f3,TYPE_UINT32,3)
)

HDU_UNIT_WITH(du9,(HDU_BASE(du8)),
    HDU_FIELD(f4,TYPE_UINT32,4)
)

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
    BOOST_CHECK_EQUAL(9,size);

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
    BOOST_CHECK_EQUAL("Hello world!",f1.value());
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

    du::WireBufChained buf3;
    auto r1=du::io::serialize(obj1,buf3);
    BOOST_CHECK(r1>0);
    BOOST_CHECK_EQUAL(r,r1);

    auto buf3Solid=buf3.toSolidWireBuf();
    BOOST_CHECK_EQUAL(buf2.mainContainer()->size(),buf3Solid.mainContainer()->size());

    type obj4;
    ok=du::io::deserialize(obj4,buf3Solid);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj4.field(du3::field4).c_str());
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
    BOOST_CHECK_EQUAL(47,size);

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

    du::WireBufChained buf3;
    auto r1=du::io::serialize(obj1,buf3);
    BOOST_CHECK(r1>0);
    BOOST_CHECK_EQUAL(r,r1);

    auto buf3Solid=buf3.toSolidWireBuf();
    BOOST_CHECK_EQUAL(buf2.mainContainer()->size(),buf3Solid.mainContainer()->size());

    type obj4;
    ok=du::io::deserialize(obj4,buf3Solid);
    BOOST_CHECK(ok);
    const auto& c4_f3=obj4.field(du4::f3).get();
    const auto& c4_f3_4=c4_f3.field(du3::field4);
    BOOST_CHECK_EQUAL("Hello world!",c4_f3_4.c_str());
}

BOOST_AUTO_TEST_CASE(SerializeRepeatedField)
{
    using traits=du5::traits;
    using type=traits::type;

    type obj1;
    auto& f5=obj1.field(du5::field5);
    f5.appendValue("Hello world!");
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

    du::WireBufChained buf3;
    auto r1=du::io::serialize(obj1,buf3);
    BOOST_CHECK(r1>0);
    BOOST_CHECK_EQUAL(r,r1);

    auto buf3Solid=buf3.toSolidWireBuf();
    BOOST_CHECK_EQUAL(buf2.mainContainer()->size(),buf3Solid.mainContainer()->size());

    type obj4;
    ok=du::io::deserialize(obj4,buf3Solid);
    BOOST_CHECK(ok);
    BOOST_CHECK_EQUAL("Hello world!",obj4.field(du5::field5).value(0).buf()->c_str());
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

BOOST_AUTO_TEST_CASE(SerializeAs)
{
    using traits=du9::traits;
    using type=traits::type;

    static_assert(type::hasId(hana::size_c<2>),"");
    static_assert(!type::hasId(hana::size_c<5>),"");

    type obj1;
    obj1.setFieldValue(du8::f2,200);
    obj1.setFieldValue(du8::f3,300);
    obj1.setFieldValue(du9::f4,400);
    BOOST_CHECK(obj1.field(du8::f2).isSet());
    BOOST_CHECK_EQUAL(obj1.fieldValue(du8::f2),200);
    BOOST_CHECK(obj1.field(du8::f3).isSet());
    BOOST_CHECK_EQUAL(obj1.fieldValue(du8::f3),300);
    BOOST_CHECK(obj1.field(du9::f4).isSet());
    BOOST_CHECK_EQUAL(obj1.fieldValue(du9::f4),400);

    HATN_NAMESPACE::Error ec;
    du::WireBufSolid buf1;
    auto r=du::io::serializeAs<du8::type>(obj1,buf1,ec);
    BOOST_CHECK(r>0);
    if (ec)
    {
        BOOST_TEST_MESSAGE(ec.message());        
    }
    BOOST_CHECK(!ec);

    type obj2;
    auto ok=du::io::deserialize(obj2,buf1);
    BOOST_CHECK(ok);

    BOOST_CHECK(obj2.field(du8::f2).isSet());
    BOOST_CHECK_EQUAL(obj2.fieldValue(du8::f2),200);
    BOOST_CHECK(obj2.field(du8::f3).isSet());
    BOOST_CHECK_EQUAL(obj2.fieldValue(du8::f3),300);
    BOOST_CHECK(!obj2.field(du9::f4).isSet());
    BOOST_CHECK_EQUAL(obj2.fieldValue(du9::f4),0);

    du::WireBufSolid buf2;
    r=du::io::serializeAs<du8::type>(obj1,buf2);
    BOOST_CHECK(r>0);

    type obj3;
    ok=du::io::deserialize(obj3,buf2);
    BOOST_CHECK(ok);

    BOOST_CHECK(obj3.field(du8::f2).isSet());
    BOOST_CHECK_EQUAL(obj3.fieldValue(du8::f2),200);
    BOOST_CHECK(obj3.field(du8::f3).isSet());
    BOOST_CHECK_EQUAL(obj3.fieldValue(du8::f3),300);
    BOOST_CHECK(!obj3.field(du9::f4).isSet());
    BOOST_CHECK_EQUAL(obj3.fieldValue(du9::f4),0);
}

BOOST_AUTO_TEST_SUITE_END()

