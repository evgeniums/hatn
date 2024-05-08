#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#define HDU_V2_UNIT_EXPORT

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/unitmeta.ipp>
#include <hatn/dataunit/detail/unittraits.ipp>

#include <hatn/dataunit/unitstrings.h>

namespace {
HDU_V2_UNIT(simple_int8,
  HDU_V2_FIELD(field2,TYPE_INT8,2)
  HDU_V2_FIELD(field3,TYPE_INT8,3)
)

HDU_V2_UNIT(simple_int16,
  HDU_V2_FIELD(field2,TYPE_INT16,2)
  HDU_V2_FIELD(field3,TYPE_INT16,3)
)

HDU_V2_UNIT(simple_int8_int16,
  HDU_V2_FIELD(field2,TYPE_INT8,2)
  HDU_V2_FIELD(field3,TYPE_INT16,3)
)

HDU_V2_UNIT(simple_int16_int8,
  HDU_V2_FIELD(field2,TYPE_INT16,3)
  HDU_V2_FIELD(field3,TYPE_INT8,2)
)

HDU_V2_UNIT(simple_string,
  HDU_V2_FIELD(field1,TYPE_STRING,1)
)
}

BOOST_AUTO_TEST_SUITE(TestFields)

BOOST_AUTO_TEST_CASE(TestBasicField)
{
    using traits=simple_int8::traits;
    using type=traits::type;

    type obj;

    auto& f1=obj.field(simple_int8::field2);

    const auto& fields=traits::fields;
    auto& f2=obj.field(fields.field2);
    static_assert(std::is_same<decltype(f1),decltype(f2)>::value,"Filed types must match");

    int8_t val=100;
    f1.set(val);
    BOOST_CHECK_EQUAL(f1.get(),val);
    BOOST_CHECK_EQUAL(f2.get(),val);

    const auto& constObj=obj;
    BOOST_CHECK_EQUAL(constObj.field(fields.field2).get(),val);
}

BOOST_AUTO_TEST_CASE(TestUnitStrings)
{
    using traits=simple_int8::traits;
    using type=traits::type;
    const auto& fields=traits::fields;

    type obj;

    BOOST_CHECK_EQUAL(std::string(obj.unitName()),std::string("simple_int8"));
    BOOST_CHECK_EQUAL(std::string(obj.unitName()),std::string(obj.name()));
    BOOST_CHECK_EQUAL(std::string(type::unitName()),std::string(obj.name()));

//    BOOST_CHECK_EQUAL(std::string(std::string(fields.field2.name)),std::string("field2"));
    BOOST_CHECK_EQUAL(std::string(obj.field(fields.field2).name()),std::string("field2"));

    auto& strings=hatn::dataunit::UnitStrings::instance();
    BOOST_CHECK(strings.isEmpty());
//    BOOST_CHECK_EQUAL(strings.fieldName<type>(fields.field2),std::string(fields.field2.name));

    std::string newName{"New name of field2"};
    strings.registerFieldName<type>(fields.field2,newName);

    BOOST_CHECK(!strings.isEmpty());
    BOOST_CHECK_EQUAL(strings.fieldName<type>(fields.field2),newName);
    BOOST_CHECK_EQUAL(strings.fieldName(&(obj.field(fields.field2))),newName);
    BOOST_CHECK_EQUAL(strings.fieldName(&obj,fields.field2.ID),newName);
    BOOST_CHECK_EQUAL(strings.fieldName(type::unitName(),fields.field2.ID),newName);

    hatn::dataunit::UnitStrings::free();
    auto& strings1=hatn::dataunit::UnitStrings::instance();
    BOOST_CHECK(strings1.isEmpty());
//    BOOST_CHECK_EQUAL(strings1.fieldName<type>(fields.field2),std::string(fields.field2.name));
}

BOOST_AUTO_TEST_CASE(TestHasField)
{
    using traits=simple_int8::traits;
    using type=traits::type;

    static_assert(type::hasField(simple_int8::field2), "Must have field as is");
    static_assert(!type::hasField(simple_int16::field2), "Must not have field");
    static_assert(!type::hasField(simple_int8_int16::field2), "Must not have field of the same type and name and ID from other unit");
    static_assert(!type::hasField(simple_int16_int8::field3), "Must not have field of the same type and ID from other unit");

    static_assert(!type::hasField(10), "Must not have illegal field");
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(TestIsSet)
{
    using traits=simple_int8::traits;
    using type=traits::type;

    type obj;

    BOOST_CHECK(!obj.isSet(simple_int8::field2));
    BOOST_CHECK(!obj.isSet(simple_int8::field3));

    obj.setFieldValue(simple_int8::field2,10);
    BOOST_CHECK(obj.isSet(simple_int8::field2));
    BOOST_CHECK(!obj.isSet(simple_int8::field3));
    BOOST_CHECK_EQUAL(obj.fieldValue(simple_int8::field2),10);

    obj.resetField(simple_int8::field2);
    BOOST_CHECK(!obj.isSet(simple_int8::field2));
    BOOST_CHECK(!obj.isSet(simple_int8::field3));
}

BOOST_AUTO_TEST_CASE(TestStringInterface)
{
    using traits=simple_string::traits;
    using type=traits::type;
    const auto& fields=traits::fields;

    type obj;

    BOOST_CHECK(!obj.isSet(fields.field1));

    obj.setFieldValue(fields.field1,"Hello world");
    BOOST_CHECK(obj.isSet(fields.field1));
    BOOST_CHECK_EQUAL(std::string(obj.field(fields.field1).c_str()),std::string("Hello world"));

    obj.resetField(fields.field1);
    BOOST_CHECK(!obj.isSet(fields.field1));
}

BOOST_AUTO_TEST_SUITE_END()
