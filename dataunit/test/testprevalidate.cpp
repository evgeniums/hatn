#include <boost/test/unit_test.hpp>

#include <hatn/validator/validator.hpp>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/dataunit/readunitfieldatpath.h>
#include <hatn/dataunit/updateunitfieldatpath.h>
#include <hatn/dataunit/prevalidatedupdate.h>

#include <hatn/validator/operators/lexicographical.hpp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

#include "testfieldpath.h"

namespace vld=HATN_VALIDATOR_NAMESPACE;

HATN_COMMON_USING
HATN_DATAUNIT_USING
namespace {

HDU_V2_UNIT(scalar_types,
    HDU_V2_FIELD(type_bool,TYPE_BOOL,1)
    HDU_V2_FIELD(type_int8,TYPE_INT8,2)
    HDU_V2_FIELD(type_int16,TYPE_INT16,3)
    HDU_V2_FIELD(type_int32,TYPE_INT32,4)
    HDU_V2_FIELD(type_int64,TYPE_INT64,5)
    HDU_V2_FIELD(type_uint8,TYPE_UINT8,6)
    HDU_V2_FIELD(type_uint16,TYPE_UINT16,7)
    HDU_V2_FIELD(type_uint32,TYPE_UINT32,8)
    HDU_V2_FIELD(type_uint64,TYPE_UINT64,9)
    HDU_V2_FIELD(type_float,TYPE_FLOAT,10)
    HDU_V2_FIELD(type_double,TYPE_DOUBLE,11)
    HDU_V2_ENUM(MyEnum,One=1,Two=2)
    HDU_V2_DEFAULT_FIELD(type_enum,HDU_V2_TYPE_ENUM(MyEnum),12,MyEnum::Two)
    )

HDU_V2_UNIT(byte_types,
    HDU_V2_FIELD(type_string,TYPE_STRING,1)
    HDU_V2_FIELD(type_bytes,TYPE_BYTES,2)
    HDU_V2_FIELD(type_fixed_string,HDU_V2_TYPE_FIXED_STRING(128),3)
    )

HDU_V2_UNIT(subunit_types,
    HDU_V2_FIELD(scalar,scalar_types::TYPE,1)
    HDU_V2_FIELD(bytes,byte_types::TYPE,2)
    )

HDU_V2_UNIT(scalar_arrays,
    HDU_V2_REPEATED_FIELD(type_int8,TYPE_INT8,2)
    HDU_V2_REPEATED_FIELD(type_int16,TYPE_INT16,3)
    HDU_V2_REPEATED_FIELD(type_int32,TYPE_INT32,4)
    HDU_V2_REPEATED_FIELD(type_int64,TYPE_INT64,5)
    HDU_V2_REPEATED_FIELD(type_uint8,TYPE_UINT8,6)
    HDU_V2_REPEATED_FIELD(type_uint16,TYPE_UINT16,7)
    HDU_V2_REPEATED_FIELD(type_uint32,TYPE_UINT32,8)
    HDU_V2_REPEATED_FIELD(type_uint64,TYPE_UINT64,9)
    HDU_V2_REPEATED_FIELD(type_float,TYPE_FLOAT,10)
    HDU_V2_REPEATED_FIELD(type_double,TYPE_DOUBLE,11)
    HDU_V2_ENUM(MyEnum,One=1,Two=2)
    HDU_V2_REPEATED_FIELD(type_enum,HDU_V2_TYPE_ENUM(MyEnum),12)
    )

HDU_V2_UNIT(buf_arrays,
    HDU_V2_REPEATED_FIELD(type_string,TYPE_STRING,1)
    HDU_V2_REPEATED_FIELD(type_bytes,TYPE_BYTES,2)
    HDU_V2_REPEATED_FIELD(type_fixed_string,HDU_V2_TYPE_FIXED_STRING(128),3)
    )

HDU_V2_UNIT(subunit_arrays,
    HDU_V2_REPEATED_FIELD(scalar,scalar_types::TYPE,1)
    HDU_V2_REPEATED_FIELD(bytes,byte_types::TYPE,2)
    )
}

BOOST_AUTO_TEST_SUITE(TestPrevalidate)

BOOST_FIXTURE_TEST_CASE(TestPrevalidatedUpdateScalarField,Env)
{
    using type=scalar_types::type;
    const auto& fields=scalar_types::traits::fields;

    vld::error_report err;
    auto v=vld::validator(
        vld::_[fields.type_int8](vld::exists,true),
        vld::_[fields.type_int8](vld::eq,100)
    );

    type obj;
    BOOST_CHECK(!v.apply(obj));

    vld::set_validated(obj,vld::_[fields.type_int8],100,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));

    vld::set_validated(obj,vld::_[fields.type_int8],10,v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));

    vld::unset_validated(obj,vld::_[fields.type_int8],v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));

    BOOST_CHECK(!obj.field(fields.type_uint8).isSet());
    vld::set_validated(obj,vld::_[fields.type_uint8],20,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    BOOST_CHECK(obj.field(fields.type_uint8).isSet());

    vld::unset_validated(obj,vld::_[fields.type_uint8],v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    BOOST_CHECK(!obj.field(fields.type_uint8).isSet());
}

BOOST_FIXTURE_TEST_CASE(TestPrevalidatedUpdateStringField,Env)
{
    using type=byte_types::type;
    const auto& fields=byte_types::traits::fields;

    vld::error_report err;
    auto v=vld::validator(
      vld::_[fields.type_string](vld::exists,true),
      vld::_[fields.type_string](vld::empty(vld::flag,false)),
      vld::_[fields.type_string](vld::size(vld::lt,20)),
      vld::_[fields.type_string](vld::lex_gte,"Hello world")
    );

    type obj;
    BOOST_CHECK(!v.apply(obj));

    vld::set_validated(obj,vld::_[fields.type_string],"Hello world",v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));

    vld::set_validated(obj,vld::_[fields.type_string],"Hello world! Hello world!",v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));
    auto val1=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val1.data(),val1.size()),std::string("Hello world"));

    vld::set_validated(obj,vld::_[fields.type_string],"Hello world!!!!!",v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    auto val2=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val2.data(),val2.size()),std::string("Hello world!!!!!"));

    vld::resize_validated(obj,vld::_[fields.type_string],50,v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));
    auto val3=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val3.data(),val3.size()),std::string("Hello world!!!!!"));

    vld::resize_validated(obj,vld::_[fields.type_string],12,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    auto val4=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val4.data(),val4.size()),std::string("Hello world!"));

    vld::resize_validated(obj,vld::_[fields.type_string],0,v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));
    auto val5=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val5.data(),val5.size()),std::string("Hello world!"));

    vld::resize_validated(obj,vld::_[fields.type_string],11,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    auto val6=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val6.data(),val6.size()),std::string("Hello world"));

    vld::set_validated(obj,vld::_[fields.type_string],"Hello world",v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));

    vld::clear_validated(obj,vld::_[fields.type_string],v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));
    auto val7=obj.getAtPath(vld::_[fields.type_string]);
    BOOST_CHECK_EQUAL(std::string(val7.data(),val7.size()),std::string("Hello world"));

    vld::set_validated(obj,vld::_[fields.type_bytes],"Hello!",v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    auto val8=obj.getAtPath(vld::_[fields.type_bytes]);
    BOOST_CHECK_EQUAL(std::string(val8.data(),val8.size()),std::string("Hello!"));

    vld::clear_validated(obj,vld::_[fields.type_bytes],v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    auto val9=obj.getAtPath(vld::_[fields.type_bytes]);
    BOOST_CHECK(val9.empty());
}

BOOST_FIXTURE_TEST_CASE(TestPrevalidatedUpdateSubunitField,Env)
{
    subunit_types::type obj;

    vld::error_report err;
    auto v=vld::validator(
        vld::_[subunit_types::scalar][scalar_types::type_int8](vld::exists,true),
        vld::_[subunit_types::scalar][scalar_types::type_int8](vld::eq,100)
    );

    BOOST_CHECK(!v.apply(obj));
    BOOST_CHECK(!obj.field(subunit_types::scalar).field(scalar_types::type_uint8).isSet());

    vld::set_validated(obj,vld::_[subunit_types::scalar][scalar_types::type_int8],100,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));

    vld::set_validated(obj,vld::_[subunit_types::scalar][scalar_types::type_int8],10,v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));

    vld::unset_validated(obj,vld::_[subunit_types::scalar][scalar_types::type_int8],v,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v.apply(obj));

    BOOST_CHECK(!obj.field(subunit_types::scalar).field(scalar_types::type_uint8).isSet());
    vld::set_validated(obj,vld::_[subunit_types::scalar][scalar_types::type_uint8],20,v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    BOOST_CHECK(obj.field(subunit_types::scalar).field(scalar_types::type_uint8).isSet());

    vld::unset_validated(obj,vld::_[subunit_types::scalar][scalar_types::type_uint8],v,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v.apply(obj));
    BOOST_CHECK(!obj.field(subunit_types::scalar).field(scalar_types::type_uint8).isSet());
}

BOOST_FIXTURE_TEST_CASE(TestPrevalidatedUpdateRepeatedSubunitField,Env)
{
    subunit_arrays::type obj;

    vld::error_report err;
    auto v1=vld::validator(
        vld::_[subunit_arrays::scalar](vld::size(vld::gte,2)),
        vld::_[subunit_arrays::scalar][1][scalar_types::type_int8](vld::exists,true),
        vld::_[subunit_arrays::scalar][1][scalar_types::type_int8](vld::eq,100)
    );
    auto v2=vld::validator(
        vld::_[subunit_arrays::scalar](vld::size(vld::gte,2)),
        vld::_[subunit_arrays::scalar][1](vld::exists,true)
    );

    BOOST_CHECK(!v1.apply(obj));
    BOOST_CHECK(!v2.apply(obj));

    obj.autoAppendAtPath(vld::_[subunit_arrays::scalar],2);
    BOOST_CHECK(!v1.apply(obj));
    BOOST_CHECK(v2.apply(obj));
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[subunit_arrays::scalar]),2);

    vld::set_validated(obj,vld::_[subunit_arrays::scalar][1][scalar_types::type_int8],100,v1,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v1.apply(obj));
    BOOST_CHECK(v2.apply(obj));

    vld::set_validated(obj,vld::_[subunit_arrays::scalar][1][scalar_types::type_int8],10,v1,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v1.apply(obj));
    BOOST_CHECK(v2.apply(obj));
}

BOOST_FIXTURE_TEST_CASE(TestPrevalidatedUpdateRepeatedScalarField,Env)
{
    using type=scalar_arrays::type;
    const auto& fields=scalar_arrays::traits::fields;
    type obj;

    vld::error_report err;
    auto v1=vld::validator(
        vld::_[fields.type_int8](vld::size(vld::gte,2)),
        vld::_[fields.type_int8][1](vld::exists,true),
        vld::_[fields.type_int8][1](vld::eq,50)
    );
    auto v2=vld::validator(
        vld::_[fields.type_int8](vld::size(vld::gte,2))
    );
    auto v3=vld::validator(
        vld::_[fields.type_int8][1](vld::exists,true)
    );
    auto v4=vld::validator(
        vld::_[fields.type_int8](vld::ALL(vld::value(vld::eq,50)))
    );
    auto v5=vld::validator(
        vld::_[fields.type_int8](vld::empty(vld::flag,true))
    );
    auto v6=vld::validator(
        vld::_[fields.type_int8](vld::empty(vld::flag,false))
    );

    BOOST_CHECK(!v1.apply(obj));
    BOOST_CHECK(!v2.apply(obj));

    //! @todo Implement validated append
    obj.appendAtPath(vld::_[fields.type_int8],100);
    obj.appendAtPath(vld::_[fields.type_int8],10);
    BOOST_CHECK(!v1.apply(obj));
    BOOST_CHECK(v2.apply(obj));
    BOOST_CHECK(v3.apply(obj));
    BOOST_CHECK(!v4.apply(obj));

    vld::set_validated(obj,vld::_[fields.type_int8][1],50,v1,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v1.apply(obj));

    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),2);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][0]),100);
    BOOST_CHECK_EQUAL(obj.getAtPath(vld::_[fields.type_int8][1]),50);

    vld::set_validated(obj,vld::_[fields.type_int8][1],30,v1,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v1.apply(obj));
    BOOST_REQUIRE_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),2);
    BOOST_CHECK_EQUAL(int(obj.getAtPath(vld::_[fields.type_int8][0])),100);
    BOOST_CHECK_EQUAL(int(obj.getAtPath(vld::_[fields.type_int8][1])),50);

    BOOST_CHECK(!v4.apply(obj));
    obj.setAtPath(vld::_[fields.type_int8][0],50);
    BOOST_CHECK(v4.apply(obj));

    // test rule ALL
    obj.appendAtPath(vld::_[fields.type_int8],1000);
    BOOST_CHECK(!v4.apply(obj));
    vld::set_validated(obj,vld::_[fields.type_int8][2],30,v4,err);
    BOOST_CHECK(err);
    BOOST_CHECK(!v4.apply(obj));
    vld::set_validated(obj,vld::_[fields.type_int8][2],50,v4,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v4.apply(obj));

    vld::resize_validated(obj,vld::_[fields.type_int8],5,v1,err);
    BOOST_CHECK(!err);
    BOOST_CHECK(v1.apply(obj));
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),5);

    vld::resize_validated(obj,vld::_[fields.type_int8],1,v1,err);
    BOOST_CHECK(err);
    BOOST_CHECK(v1.apply(obj));
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),5);

    vld::clear_validated(obj,vld::_[fields.type_int8],v6,err);
    BOOST_CHECK(err);
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),5);

    vld::clear_validated(obj,vld::_[fields.type_int8],v5,err);
    BOOST_CHECK(!err);
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),0);

    vld::resize_validated(obj,vld::_[fields.type_int8],5,v5,err);
    BOOST_CHECK(err);
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),0);

    vld::resize_validated(obj,vld::_[fields.type_int8],5,v6,err);
    BOOST_CHECK(!err);
    BOOST_CHECK_EQUAL(obj.sizeAtPath(vld::_[fields.type_int8]),5);
}

BOOST_AUTO_TEST_CASE(TestValidatorWithNestedSize)
{
    auto v1=vld::validator(
                vld::_[scalar_arrays::type_int8][vld::size](vld::gte,2)
            );
    scalar_arrays::type obj1;
    BOOST_CHECK(!v1.apply(obj1));

    auto v2=vld::validator(
                vld::_[byte_types::type_string][vld::size](vld::gte,2)
            );
    byte_types::type obj2;
    BOOST_CHECK(!v2.apply(obj2));
}

BOOST_AUTO_TEST_SUITE_END()
