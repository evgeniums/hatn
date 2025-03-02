#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#define HDU_V2_UNIT_EXPORT

#include <hatn/common/bytearray.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/dataunit/unitstrings.h>

namespace {
HDU_V2_UNIT(simple_int32,
  HDU_V2_FIELD(field2,TYPE_INT32,2,false,0)
  HDU_V2_FIELD(field3,TYPE_INT32,3,false,0)
)

HDU_V2_UNIT(with_dynamic_subunit,
  HDU_V2_FIELD(field1,TYPE_INT32,1)
  HDU_V2_FIELD(sub,TYPE_DATAUNIT,2)
)

}

BOOST_AUTO_TEST_SUITE(TestDynamicSubunit)

BOOST_AUTO_TEST_CASE(TestSerDeser)
{
    // fill subunit1
    simple_int32::type subunit1;
    subunit1.setFieldValue(simple_int32::field2,200);
    subunit1.setFieldValue(simple_int32::field3,300);
    // serialize subunit1 as subunit
    HATN_DATAUNIT_NAMESPACE::WireBufSolid wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serializeAsSubunit(subunit1,wbuf1,with_dynamic_subunit::sub.id());
    BOOST_REQUIRE_GT(packedSize,0);
    // serialize subunit1 as unit for sample
    HATN_DATAUNIT_NAMESPACE::WireBufSolid wbuf1Sample;
    packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(subunit1,wbuf1Sample);
    BOOST_REQUIRE_GT(packedSize,0);

    // fill and serialize obj1 without subunit
    HATN_DATAUNIT_NAMESPACE::WireDataSingle wbuf2;
    with_dynamic_subunit::type obj1;
    obj1.setFieldValue(with_dynamic_subunit::field1,100);
    packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(obj1,wbuf2);
    BOOST_REQUIRE_GT(packedSize,0);

    // merge serialized data of obj1 and subunit1
    HATN_COMMON_NAMESPACE::ByteArray mergedBuf{*wbuf2.mainContainer()};
    mergedBuf.append(*wbuf1.mainContainer());
    HATN_DATAUNIT_NAMESPACE::WireBufSolid mergedWbuf{mergedBuf};

    // set subunit field of obj2 to keep skipped subunit data when deserializing
    with_dynamic_subunit::type obj2;
    auto& subField2=obj2.field(with_dynamic_subunit::sub);
    BOOST_CHECK(!subField2.skippedNotParsedContent());
    HATN_COMMON_NAMESPACE::ByteArrayShared subBuf2=HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>();
    subField2.keepContentInBufInsteadOfParsing(subBuf2);
    BOOST_CHECK(subField2.skippedNotParsedContent());
    BOOST_CHECK(subBuf2->empty());

    // deserialize obj2 from merged buffer
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj2,mergedWbuf);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(obj2.fieldValue(with_dynamic_subunit::field1),100);
    BOOST_CHECK(obj2.isSet(with_dynamic_subunit::sub));
    BOOST_REQUIRE(!subBuf2->empty());
    BOOST_CHECK_EQUAL(wbuf1Sample.mainContainer()->size(),subBuf2->size());
    BOOST_CHECK(*wbuf1Sample.mainContainer()==*subBuf2);

    // deserialize subunit2 from container of deserialized subunit data
    HATN_DATAUNIT_NAMESPACE::WireBufSolidShared subWbuf2{subBuf2};
    simple_int32::type subunit2;
    ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(subunit2,subWbuf2);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(static_cast<int>(subunit2.fieldValue(simple_int32::field2)),200);
    BOOST_CHECK_EQUAL(static_cast<int>(subunit2.fieldValue(simple_int32::field3)),300);

    // deserialize obj3 from wbuf2 without subunit
    with_dynamic_subunit::type obj3;
    ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj3,wbuf2);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(obj3.fieldValue(with_dynamic_subunit::field1),100);
    BOOST_CHECK(!obj3.isSet(with_dynamic_subunit::sub));
}

BOOST_AUTO_TEST_CASE(TestWithSubunit)
{
    // fill subunit1
    auto subunit1=HATN_COMMON_NAMESPACE::makeShared<simple_int32::managed>();
    subunit1->setFieldValue(simple_int32::field2,200);
    subunit1->setFieldValue(simple_int32::field3,300);

    // fill and serialize obj1 with subunit
    with_dynamic_subunit::type obj1;
    obj1.setFieldValue(with_dynamic_subunit::field1,100);
    obj1.setFieldValue(with_dynamic_subunit::sub,subunit1);
    HATN_DATAUNIT_NAMESPACE::WireDataSingle wbuf1;
    auto packedSize=HATN_DATAUNIT_NAMESPACE::io::serialize(obj1,wbuf1);
    BOOST_REQUIRE_GT(packedSize,0);

    // deserialize to obj2
    with_dynamic_subunit::type obj2;
    auto subunit2=HATN_COMMON_NAMESPACE::makeShared<simple_int32::managed>();
    obj2.setFieldValue(with_dynamic_subunit::sub,subunit2);
    auto ok=HATN_DATAUNIT_NAMESPACE::io::deserialize(obj2,wbuf1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(obj2.fieldValue(with_dynamic_subunit::field1),100);
    BOOST_CHECK(obj2.isSet(with_dynamic_subunit::sub));
    BOOST_CHECK_EQUAL(subunit2->fieldValue(simple_int32::field2),200);
    BOOST_CHECK_EQUAL(subunit2->fieldValue(simple_int32::field3),300);
}

BOOST_AUTO_TEST_SUITE_END()
