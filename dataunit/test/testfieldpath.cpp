#include <boost/test/unit_test.hpp>

#include <hatn/validator/validator.hpp>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC
#define HDU_DATAUNIT_EXPORT

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#include <hatn/dataunit/readunitfieldatpath.h>
#include <hatn/dataunit/updateunitfieldatpath.h>
#include <hatn/dataunit/prevalidatedupdate.h>

#include <hatn/validator/operators/lexicographical.hpp>

#include "testfieldpath.h"

namespace vld=HATN_VALIDATOR_NAMESPACE;

HATN_COMMON_USING
HATN_DATAUNIT_USING

namespace {

HDU_DATAUNIT(scalar_types,
    HDU_FIELD(type_bool,TYPE_BOOL,1)
    HDU_FIELD(type_int8,TYPE_INT8,2)
    HDU_FIELD(type_int16,TYPE_INT16,3)
    HDU_FIELD(type_int32,TYPE_INT32,4)
    HDU_FIELD(type_int64,TYPE_INT64,5)
    HDU_FIELD(type_uint8,TYPE_UINT8,6)
    HDU_FIELD(type_uint16,TYPE_UINT16,7)
    HDU_FIELD(type_uint32,TYPE_UINT32,8)
    HDU_FIELD(type_uint64,TYPE_UINT64,9)
    HDU_FIELD(type_float,TYPE_FLOAT,10)
    HDU_FIELD(type_double,TYPE_DOUBLE,11)
    HDU_ENUM(MyEnum,One=1,Two=2)
    HDU_FIELD_DEFAULT(type_enum,HDU_TYPE_ENUM(MyEnum),12,MyEnum::Two)
)

HDU_DATAUNIT(byte_types,
    HDU_FIELD(type_string,TYPE_STRING,1)
    HDU_FIELD(type_bytes,TYPE_BYTES,2)
    HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(128),3)
)

HDU_DATAUNIT(subunit_types,
    HDU_FIELD_DATAUNIT(scalar,scalar_types::TYPE,1)
    HDU_FIELD_DATAUNIT(bytes,byte_types::TYPE,2)
)

HDU_DATAUNIT(scalar_arrays,
    HDU_FIELD_REPEATED(type_int8,TYPE_INT8,2)
    HDU_FIELD_REPEATED(type_int16,TYPE_INT16,3)
    HDU_FIELD_REPEATED(type_int32,TYPE_INT32,4)
    HDU_FIELD_REPEATED(type_int64,TYPE_INT64,5)
    HDU_FIELD_REPEATED(type_uint8,TYPE_UINT8,6)
    HDU_FIELD_REPEATED(type_uint16,TYPE_UINT16,7)
    HDU_FIELD_REPEATED(type_uint32,TYPE_UINT32,8)
    HDU_FIELD_REPEATED(type_uint64,TYPE_UINT64,9)
    HDU_FIELD_REPEATED(type_float,TYPE_FLOAT,10)
    HDU_FIELD_REPEATED(type_double,TYPE_DOUBLE,11)
    HDU_ENUM(MyEnum,One=1,Two=2)
    HDU_FIELD_REPEATED(type_enum,HDU_TYPE_ENUM(MyEnum),12)
)

HDU_DATAUNIT(buf_arrays,
    HDU_FIELD_REPEATED(type_string,TYPE_STRING,1)
    HDU_FIELD_REPEATED(type_bytes,TYPE_BYTES,2)
    HDU_FIELD_REPEATED(type_fixed_string,HDU_TYPE_FIXED_STRING(128),3)
)

HDU_DATAUNIT(subunit_arrays,
    HDU_FIELD_REPEATED_DATAUNIT(scalar,scalar_types::TYPE,1)
    HDU_FIELD_REPEATED_DATAUNIT(bytes,byte_types::TYPE,2)
)

HDU_INSTANTIATE_DATAUNIT(scalar_types)
HDU_INSTANTIATE_DATAUNIT(byte_types)
HDU_INSTANTIATE_DATAUNIT(subunit_types)
HDU_INSTANTIATE_DATAUNIT(scalar_arrays)
HDU_INSTANTIATE_DATAUNIT(buf_arrays)
HDU_INSTANTIATE_DATAUNIT(subunit_arrays)

}

// constexpr const auto& _=vld::_;

BOOST_AUTO_TEST_SUITE(TestFieldPath)

BOOST_FIXTURE_TEST_CASE(TestReadScalarField,Env)
{
    using type=scalar_types::type;
    const auto& fields=scalar_types::traits::fields;

    type obj;

    obj.field(fields.type_int8).set(100);
    BOOST_CHECK_EQUAL(int(obj[fields.type_int8]),100);
    auto val=getUnitFieldAtPath(obj,vld::_[fields.type_int8]);
    BOOST_CHECK_EQUAL(int(val),100);

    auto v1=vld::validator(
                vld::_[fields.type_int8](vld::eq,100)
           );
    BOOST_CHECK(v1.apply(obj));

    auto v2=vld::validator(
                vld::_[fields.type_int8](vld::eq,10)
           );
    BOOST_CHECK(!v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestReadStringField,Env)
{
    using type=byte_types::type;
    const auto& fields=byte_types::traits::fields;

    type obj;

    obj.field(fields.type_string).set("Hello world");

    auto val1=obj[fields.type_string];
    BOOST_CHECK_EQUAL(std::string(val1.data(),val1.size()),std::string("Hello world"));

    auto val2=getUnitFieldAtPath(obj,vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val2.data(),val2.size()),std::string("Hello world"));

    BOOST_CHECK_EQUAL(getUnitFieldAtPath(obj,vld::_[fields.type_string][1]),'e');

    auto v1=vld::validator(
                vld::_[fields.type_string](vld::lex_eq,"Hello world")
           );
    BOOST_CHECK(v1.apply(obj));

    auto v2=vld::validator(
                vld::_[fields.type_string](vld::lex_eq,"Other string")
           );
    BOOST_CHECK(!v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestReadSubunitField,Env)
{
    subunit_types::type obj;

    obj.field(subunit_types::scalar).field(scalar_types::type_int8).set(100);

    auto val=getUnitFieldAtPath(obj,vld::_[subunit_types::scalar][scalar_types::type_int8]);
    BOOST_CHECK_EQUAL(int(val),100);

    auto v1=vld::validator(
                vld::_[subunit_types::scalar][scalar_types::type_int8](vld::eq,100)
           );
    BOOST_CHECK(v1.apply(obj));

    auto v2=vld::validator(
                vld::_[subunit_types::scalar][scalar_types::type_int8](vld::eq,10)
           );
    BOOST_CHECK(!v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestReadRepeatedScalarField,Env)
{
    using type=scalar_arrays::type;
    const auto& fields=scalar_arrays::traits::fields;

    type obj;

    obj.field(fields.type_int8).addValue(100);
    obj.field(fields.type_int8).addValue(10);

    obj.field(fields.type_int8).setValue(0,50);

    auto val=getUnitFieldAtPath(obj,vld::_[fields.type_int8][1]);
    BOOST_CHECK_EQUAL(int(val),10);

    auto v1=vld::validator(
                vld::_[fields.type_int8][0](vld::eq,50),
                vld::_[fields.type_int8][1](vld::eq,10)
           );
    BOOST_CHECK(v1.apply(obj));

    auto v2=vld::validator(
                vld::_[fields.type_int8][1](vld::eq,100)
           );
    BOOST_CHECK(!v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestReadRepeatedSubunitField,Env)
{
    subunit_arrays::type obj;

    obj.field(subunit_arrays::scalar).addValue(scalar_types::type());
    obj.field(subunit_arrays::scalar).addValue(scalar_types::type());
    obj.field(subunit_arrays::scalar).field(1).field(scalar_types::type_int8).set(100);

    auto val=getUnitFieldAtPath(obj,vld::_[subunit_arrays::scalar][1][scalar_types::type_int8]);
    BOOST_CHECK_EQUAL(int(val),100);

    auto v1=vld::validator(
                vld::_[subunit_arrays::scalar][1][scalar_types::type_int8](vld::eq,100)
           );
    BOOST_CHECK(v1.apply(obj));

    auto v2=vld::validator(
                vld::_[subunit_arrays::scalar][1][scalar_types::type_int8](vld::eq,10)
           );
    BOOST_CHECK(!v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestUpdateScalarField,Env)
{
    using type=scalar_types::type;
    const auto& fields=scalar_types::traits::fields;

    type obj;
    BOOST_CHECK(!obj.field(fields.type_int8).isSet());

    obj.setAtPath(vld::_[fields.type_int8],100);
    BOOST_CHECK(obj.field(fields.type_int8).isSet());
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8]),100);

    obj.unsetAtPath(vld::_[fields.type_int8]);
    BOOST_CHECK(!obj.field(fields.type_int8).isSet());
}

BOOST_FIXTURE_TEST_CASE(TestUpdateStringField,Env)
{
    using type=byte_types::type;
    const auto& fields=byte_types::traits::fields;

    type obj;
    BOOST_CHECK(obj.field(fields.type_string).buf()->isEmpty());

    obj.setAtPath(vld::_[fields.type_string],"Hello world");
    BOOST_CHECK(!obj.field(fields.type_string).buf()->isEmpty());
    auto val1=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val1.data(),val1.size()),std::string("Hello world"));
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_string]),std::string("Hello world").size());

    obj.resizeAtPath(vld::_[fields.type_string],5);
    auto val2=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val2.data(),val2.size()),std::string("Hello"));
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_string]),std::string("Hello").size());

    obj.reserveAtPath(vld::_[fields.type_string],500);
    BOOST_CHECK_GE(obj.field(fields.type_string).buf()->capacity(),static_cast<size_t>(500));

    obj.appendAtPath(vld::_[fields.type_string]," world");
    auto val3=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val3.data(),val3.size()),std::string("Hello world"));
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_string]),std::string("Hello world").size());

    obj.clearAtPath(vld::_[fields.type_string]);
    BOOST_CHECK(obj.field(fields.type_string).buf()->isEmpty());
}

BOOST_FIXTURE_TEST_CASE(TestUpdateRepeatedScalarField,Env)
{
    using type=scalar_arrays::type;
    const auto& fields=scalar_arrays::traits::fields;

    type obj;
    BOOST_CHECK(!obj.field(fields.type_int8).isSet());

    obj.appendAtPath(vld::_[fields.type_int8],100);
    BOOST_CHECK(obj.field(fields.type_int8).isSet());
    obj.appendAtPath(vld::_[fields.type_int8],10);

    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),2);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][0]),100);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][1]),10);

    obj.setAtPath(vld::_[fields.type_int8][1],50);
    obj.setAtPath(vld::_[fields.type_int8][0],30);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][0]),30);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][1]),50);

    obj.appendAtPath(vld::_[fields.type_int8],60,70,80);
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),5);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][0]),30);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][1]),50);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][2]),60);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][3]),70);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][4]),80);

    obj.resizeAtPath(vld::_[fields.type_int8],3);
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),3);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][0]),30);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][1]),50);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][2]),60);

    obj.reserveAtPath(vld::_[fields.type_int8],500);
    BOOST_CHECK_GE(obj.field(fields.type_int8).capacity(),static_cast<size_t>(500));

    obj.autoAppendAtPath(vld::_[fields.type_int8],7);
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),10);

    obj.clearAtPath(vld::_[fields.type_int8]);
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),0);
}

BOOST_FIXTURE_TEST_CASE(TestUpdateSubunitField,Env)
{
    subunit_types::type obj;

    BOOST_CHECK(!obj.field(subunit_types::scalar).field(scalar_types::type_int8).isSet());

    obj.setAtPath(vld::_[subunit_types::scalar][scalar_types::type_int8],100);
    BOOST_CHECK(obj.field(subunit_types::scalar).field(scalar_types::type_int8).isSet());
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[subunit_types::scalar][scalar_types::type_int8]),100);

    obj.unsetAtPath(vld::_[subunit_types::scalar][scalar_types::type_int8]);
    BOOST_CHECK(!obj.field(subunit_types::scalar).field(scalar_types::type_int8).isSet());
}

BOOST_FIXTURE_TEST_CASE(TestUpdateRepeatedSubunitField,Env)
{
    subunit_arrays::type obj;

    BOOST_CHECK(!obj.field(subunit_arrays::scalar).isSet());
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[subunit_arrays::scalar]),0);
    obj.autoAppendAtPath(vld::_[subunit_arrays::scalar],2);
    BOOST_CHECK(obj.field(subunit_arrays::scalar).isSet());
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[subunit_arrays::scalar]),2);

    obj.setAtPath(vld::_[subunit_arrays::scalar][1][scalar_types::type_int8],100);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[subunit_arrays::scalar][1][scalar_types::type_int8]),100);
}

BOOST_AUTO_TEST_SUITE_END()
