#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#define HDU_V2_UNIT_EXPORT

#include <hatn/validator/member.hpp>

#include <hatn/common/bytearray.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/compare.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/dataunit/unitstrings.h>

namespace {
HDU_V2_UNIT(u1,
  HDU_V2_FIELD(field2,TYPE_INT32,2,false,0)
  HDU_V2_FIELD(field3,TYPE_INT32,3,false,0)
)

HDU_V2_UNIT(u2,
  HDU_V2_FIELD(field1,TYPE_INT32,1)
  HDU_V2_FIELD(sub,u1::TYPE,2)
)

HDU_V2_UNIT(u3,
    HDU_V2_FIELD(field1,TYPE_INT32,1)
    HDU_V2_REPEATED_FIELD(rep,u1::TYPE,2)
)

HDU_V2_UNIT(u4,
    HDU_V2_FIELD(field1,TYPE_INT32,1)
    HDU_V2_FIELD(sub1,u2::TYPE,2)
)

HDU_V2_UNIT(u5,
    HDU_V2_FIELD(field1,TYPE_INT32,1)
    HDU_V2_FIELD(sub1,TYPE_DATAUNIT,2)
)

}

BOOST_AUTO_TEST_SUITE(TesSubunit)

BOOST_AUTO_TEST_CASE(ExplicitSubunit)
{
    // fill subunit1
    auto subunit1=HATN_COMMON_NAMESPACE::makeShared<u1::managed>();
    subunit1->setFieldValue(u1::field2,200);
    subunit1->setFieldValue(u1::field3,300);

    // fill and serialize obj1 with subunit
    u2::type obj1;
    obj1.setFieldValue(u2::field1,100);
    obj1.setFieldValue(u2::sub,subunit1);
    HATN_DATAUNIT_NAMESPACE::WireDataSingle wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(obj1,wbuf1);
    BOOST_REQUIRE_GT(packedSize,0);

    // deserialize to obj2
    u2::type obj2;
    auto subunit2=HATN_COMMON_NAMESPACE::makeShared<u1::managed>();
    obj2.setFieldValue(u2::sub,subunit2);
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj2,wbuf1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(obj2.fieldValue(u2::field1),100);
    BOOST_CHECK(obj2.isSet(u2::sub));
    BOOST_CHECK_EQUAL(subunit2->fieldValue(u1::field2),200);
    BOOST_CHECK_EQUAL(subunit2->fieldValue(u1::field3),300);
}

BOOST_AUTO_TEST_CASE(AutoSharedSubunit)
{
    // fill and serialize obj1 with subunit
    u2::type obj1;
    BOOST_CHECK(obj1.field(u2::sub).isNull());
    BOOST_CHECK(!obj1.field(u2::sub).isSharedValue());
    BOOST_CHECK(!obj1.field(u2::sub).isPlainValue());
    BOOST_CHECK(obj1.field(u2::sub).isShared());
    BOOST_CHECK(!obj1.isSet(u2::sub));

    obj1.setFieldValue(u2::field1,100);
    obj1.field(u2::sub).mutableValue()->setFieldValue(u1::field2,200);
    BOOST_CHECK(!obj1.field(u2::sub).isNull());
    BOOST_CHECK(obj1.field(u2::sub).isSharedValue());
    BOOST_CHECK(!obj1.field(u2::sub).isPlainValue());
    BOOST_CHECK(obj1.isSet(u2::sub));
    obj1.field(u2::sub).mutableValue()->setFieldValue(u1::field3,300);

    HATN_COMMON_NAMESPACE::SharedPtr<u1::managed> subunit1=obj1.field(u2::sub).sharedValue();
    BOOST_REQUIRE(!subunit1.isNull());
    BOOST_CHECK_EQUAL(subunit1->fieldValue(u1::field2),200);
    BOOST_CHECK_EQUAL(subunit1->fieldValue(u1::field3),300);

    HATN_DATAUNIT_NAMESPACE::WireDataSingle wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(obj1,wbuf1);
    BOOST_REQUIRE_GT(packedSize,0);

    // deserialize to obj2
    u2::type obj2;
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj2,wbuf1);
    BOOST_REQUIRE(ok);

    BOOST_CHECK(!obj2.field(u2::sub).isNull());
    BOOST_CHECK(obj2.field(u2::sub).isSharedValue());
    BOOST_CHECK(!obj2.field(u2::sub).isPlainValue());
    BOOST_CHECK(obj2.field(u2::sub).isShared());

    BOOST_CHECK_EQUAL(obj2.fieldValue(u2::field1),100);
    BOOST_CHECK(obj2.isSet(u2::sub));
    BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field2),200);
    BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field3),300);

    HATN_COMMON_NAMESPACE::SharedPtr<u1::managed> subunit2=obj2.field(u2::sub).sharedValue();
    BOOST_REQUIRE(!subunit2.isNull());
    BOOST_CHECK_EQUAL(subunit2->fieldValue(u1::field2),200);
    BOOST_CHECK_EQUAL(subunit2->fieldValue(u1::field3),300);

    auto checkConst=[](const auto& obj2)
    {
        BOOST_CHECK_EQUAL(obj2.fieldValue(u2::field1),100);
        BOOST_CHECK(obj2.isSet(u2::sub));
        BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field2),200);
        BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field3),300);
    };
    checkConst(obj2);
}

BOOST_AUTO_TEST_CASE(AutoPlainSubunit)
{
    // fill and serialize obj1 with subunit
    u2::type obj1;
    BOOST_CHECK(obj1.field(u2::sub).isNull());
    BOOST_CHECK(!obj1.field(u2::sub).isSharedValue());
    BOOST_CHECK(!obj1.field(u2::sub).isPlainValue());
    BOOST_CHECK(obj1.field(u2::sub).isShared());
    obj1.setSharedSubunits(false);
    BOOST_CHECK(!obj1.field(u2::sub).isShared());
    obj1.setSharedSubunits(true);
    BOOST_CHECK(obj1.field(u2::sub).isShared());
    obj1.field(u2::sub).setShared(false,false);
    BOOST_CHECK(!obj1.field(u2::sub).isShared());
    BOOST_CHECK(!obj1.isSet(u2::sub));

    obj1.setFieldValue(u2::field1,100);
    obj1.field(u2::sub).mutableValue()->setFieldValue(u1::field2,200);
    BOOST_CHECK(!obj1.field(u2::sub).isNull());
    BOOST_CHECK(!obj1.field(u2::sub).isSharedValue());
    BOOST_CHECK(obj1.field(u2::sub).isPlainValue());
    BOOST_CHECK(obj1.isSet(u2::sub));
    obj1.field(u2::sub).mutableValue()->setFieldValue(u1::field3,300);

    HATN_COMMON_NAMESPACE::SharedPtr<u1::managed> subunit1=obj1.field(u2::sub).sharedValue();
    BOOST_CHECK(subunit1.isNull());

    HATN_DATAUNIT_NAMESPACE::WireDataSingle wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(obj1,wbuf1);
    BOOST_REQUIRE_GT(packedSize,0);

    // deserialize to obj2
    u2::type obj2;
    obj2.setSharedSubunits(false);
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj2,wbuf1);
    BOOST_REQUIRE(ok);

    BOOST_CHECK(!obj2.field(u2::sub).isNull());
    BOOST_CHECK(!obj2.field(u2::sub).isSharedValue());
    BOOST_CHECK(obj2.field(u2::sub).isPlainValue());
    BOOST_CHECK(!obj2.field(u2::sub).isShared());

    BOOST_CHECK_EQUAL(obj2.fieldValue(u2::field1),100);
    BOOST_CHECK(obj2.isSet(u2::sub));
    BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field2),200);
    BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field3),300);

    HATN_COMMON_NAMESPACE::SharedPtr<u1::managed> subunit2=obj2.field(u2::sub).sharedValue();
    BOOST_CHECK(subunit2.isNull());

    auto checkConst=[](const auto& obj2)
    {
        BOOST_CHECK_EQUAL(obj2.fieldValue(u2::field1),100);
        BOOST_CHECK(obj2.isSet(u2::sub));
        BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field2),200);
        BOOST_CHECK_EQUAL(obj2.field(u2::sub).value().fieldValue(u1::field3),300);
    };
    checkConst(obj2);
}

BOOST_AUTO_TEST_CASE(RepeatedSubunit)
{
    u3::type o1;

    auto& repField=o1.field(u3::rep);

    auto v1=HATN_COMMON_NAMESPACE::makeShared<u1::managed>();
    repField.append(std::move(v1));
    BOOST_CHECK(repField._(0).isSharedValue());
    repField.append(u1::type{});
    BOOST_CHECK(repField._(1).isPlainValue());
    auto& sub2=repField.appendSharedSubunit();
    BOOST_CHECK(sub2.isSharedValue());
    auto& sub3=repField.appendPlainSubunit();
    BOOST_CHECK(sub3.isPlainValue());

    BOOST_CHECK_EQUAL(repField.count(),4);
}

BOOST_AUTO_TEST_CASE(NestedAccessors)
{
    u4::type u4;

    BOOST_CHECK(u4.member(u4::sub1,u2::sub,u1::field2)==nullptr);

    u4.mutableMember(u4::sub1,u2::sub,u1::field2)->set(100);
    BOOST_REQUIRE(u4.member(u4::sub1,u2::sub,u1::field2)!=nullptr);
    BOOST_CHECK_EQUAL(u4.member(u4::sub1,u2::sub,u1::field2)->value(),100);

    auto sub2=u4.member(u4::sub1,u2::sub)->sharedValue();
    BOOST_CHECK_EQUAL(sub2->field(u1::field2).value(),100);
}

BOOST_AUTO_TEST_CASE(SerializeAbstractSubunit)
{
    u5::type u1;

    auto subunit1=HATN_COMMON_NAMESPACE::makeShared<u1::managed>();
    u1.field(u5::sub1).set(subunit1);
    u1.field(u5::field1).set(100);

    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(u1,wbuf1);
    BOOST_REQUIRE_GT(packedSize,0);

    u5::type u2;
    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared wbuf2(wbuf1.sharedMainContainer());
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(u2,wbuf2);
    BOOST_REQUIRE(ok);

    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&u2,&u1));
    u2.field(u5::field1).set(200);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&u2,&u1));
    BOOST_REQUIRE(u2.field(u5::sub1).isSet());
    BOOST_REQUIRE(!u2.field(u5::sub1).skippedNotParsedContent().isNull());

    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared wbuf3;
    packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(u2,wbuf3);
    BOOST_REQUIRE_GT(packedSize,0);

    u5::type u3;
    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared wbuf4(wbuf3.sharedMainContainer());
    ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(u3,wbuf4);
    BOOST_REQUIRE(ok);

    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&u2,&u3));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&u1,&u3));
}

BOOST_AUTO_TEST_CASE(CompareSubunits)
{
    u1::type o1;
    u1::type o2;
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2));
    o1.setFieldValue(u1::field2,200);
    o1.setFieldValue(u1::field3,300);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o1,&o2));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(&o2,&o1));
    o2.setFieldValue(u1::field2,200);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o1,&o2));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(&o2,&o1));
    o2.setFieldValue(u1::field3,300);
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2));
    o2.setFieldValue(u1::field3,400);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(&o1,&o2));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o2,&o1));

    std::cout << "Compare with excluded field " << u1::field3.name() << " id " << u1::field3.id() << std::endl;
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2,u1::field3));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o1,&o2,u1::field3));
    o1.setFieldValue(u1::field3,400);
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o1,&o2,u2::sub));

    u2::type o21;
    u2::type o22;
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(decltype(&o21){nullptr},&o21));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o21,decltype(&o22){nullptr}));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(decltype(&o21){nullptr},&o21));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o21,decltype(&o22){nullptr}));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(decltype(&o21){nullptr},decltype(&o22){nullptr}));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o21,&o22));

    o21.mutableMember(u2::sub,u1::field2)->set(100);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o21,&o22));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o21,&o22));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(&o22,&o21));
    o22.mutableMember(u2::sub,u1::field2)->set(100);
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsEqual(&o21,&o22));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o21,&o22));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o22,&o21));
    o22.mutableMember(u2::sub,u1::field2)->set(200);
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsEqual(&o21,&o22));
    BOOST_CHECK(HATN_DATAUNIT_NAMESPACE::unitsLess(&o21,&o22));
    BOOST_CHECK(!HATN_DATAUNIT_NAMESPACE::unitsLess(&o22,&o21));
}

BOOST_AUTO_TEST_SUITE_END()
