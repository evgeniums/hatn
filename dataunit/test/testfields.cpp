#include <boost/test/unit_test.hpp>
#include <boost/hana.hpp>

#define HDU_DATAUNIT_EXPORT

#include <hatn/common/elapsedtimer.h>

#include <hatn/common/pmr/withstaticallocator.h>
//#define HATN_WITH_STATIC_ALLOCATOR_SRC
#ifdef HATN_WITH_STATIC_ALLOCATOR_SRC
    #include <hatn/common/pmr/withstaticallocator.ipp>
    #define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC
#else
    #define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_H
#endif

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>
#include <hatn/dataunit/unitstrings.h>

namespace {
HDU_DATAUNIT(simple_int8,
  HDU_FIELD(field2,TYPE_INT8,2)
  HDU_FIELD(field3,TYPE_INT8,3)
)
HDU_INSTANTIATE_DATAUNIT(simple_int8)

HDU_DATAUNIT(simple_int16,
  HDU_FIELD(field2,TYPE_INT16,2)
  HDU_FIELD(field3,TYPE_INT16,3)
)
HDU_INSTANTIATE_DATAUNIT(simple_int16)

HDU_DATAUNIT(simple_int8_int16,
  HDU_FIELD(field2,TYPE_INT8,2)
  HDU_FIELD(field3,TYPE_INT16,3)
)
HDU_INSTANTIATE_DATAUNIT(simple_int8_int16)

HDU_DATAUNIT(simple_int16_int8,
  HDU_FIELD(field2,TYPE_INT16,3)
  HDU_FIELD(field3,TYPE_INT8,2)
)
HDU_INSTANTIATE_DATAUNIT(simple_int16_int8)

HDU_DATAUNIT(simple_string,
  HDU_FIELD(field1,TYPE_STRING,1)
)
HDU_INSTANTIATE_DATAUNIT(simple_string)

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
    static_assert(type::hasField(simple_int8_int16::field2), "Must have field of the same type and name and ID from other unit");
    static_assert(type::hasField(simple_int16_int8::field3), "Must have field of the same type and ID from other unit");

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

    obj.clearField(simple_int8::field2);
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

    obj.clearField(fields.field1);
    BOOST_CHECK(!obj.isSet(fields.field1));
}

BOOST_AUTO_TEST_SUITE_END()

#if 0
BOOST_AUTO_TEST_SUITE(DataUnit)

BOOST_AUTO_TEST_CASE(TestBasic)
{
    using traits=simple_int8::traits;
    using fields=traits::fields;
    using type=traits::type;

    type obj;

    BOOST_TEST_CONTEXT("Normal construct")
    {
    }

    BOOST_TEST_CONTEXT("Copy construct")
    {
        auto copyObj=obj;
    }

    BOOST_TEST_CONTEXT("Copy operator")
    {
        type assignObj;
        assignObj=obj;
    HATN_DATAUNIT_NAMESPACE_END

BOOST_AUTO_TEST_SUITE_END()
#endif
#if 0

#ifdef _WIN32
#define HATN_DATAUNIT_EXPORT __declspec(dllexport)
#else
#define HATN_DATAUNIT_EXPORT
#endif

//HDU_FIELD(type_bool,TYPE_BOOL,1)
//HDU_FIELD(type_int8,TYPE_INT8,2)
//HDU_FIELD(type_int16,TYPE_INT16,3)
//HDU_FIELD(type_int32,TYPE_INT32,4)
//HDU_FIELD(type_int64,TYPE_INT64,5)
//HDU_FIELD(type_uint8,TYPE_UINT8,6)
//HDU_FIELD(type_uint16,TYPE_UINT16,7)
//HDU_FIELD(type_uint32,TYPE_UINT32,8)
//HDU_FIELD(type_uint64,TYPE_UINT64,9)
//HDU_FIELD(type_float,TYPE_FLOAT,10)
//HDU_FIELD(f1,TYPE_BOOL,11)
//HDU_FIELD(f2,TYPE_INT8,12)
//HDU_FIELD(f3,TYPE_INT16,13)
//HDU_FIELD(f4,TYPE_INT32,14)
//HDU_FIELD(f5,TYPE_INT64,15)
//HDU_FIELD(f6,TYPE_UINT8,16)
//HDU_FIELD(f7,TYPE_UINT16,17)
//HDU_FIELD(f8,TYPE_UINT32,18)
//HDU_FIELD(f9,TYPE_UINT64,19)

HDU_DATAUNIT(all_types,
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
            HDU_FIELD(type_string,TYPE_STRING,12)
            HDU_FIELD(type_bytes,TYPE_BYTES,13)
            HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(8),20)
            HDU_FIELD_REQUIRED(type_int8_required,TYPE_INT8,25)
            HDU_ENUM(MyEnum,One=1,Two=2)
)

HDU_DATAUNIT(bt,
            HDU_FIELD(type_bool,TYPE_BOOL,1)
            HDU_FIELD(type_int8,TYPE_INT8,2)
            HDU_FIELD(type_int16,TYPE_INT16,3)
            HDU_FIELD(type_int32,TYPE_INT32,4)
            HDU_FIELD(type_int64,TYPE_INT64,5)
            HDU_FIELD(type_uint8,TYPE_UINT8,6)
            HDU_FIELD(f9,TYPE_UINT64,19)
            HDU_FIELD(type_string,TYPE_STRING,12)
            HDU_FIELD(type_bytes,TYPE_BYTES,13)
            HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(8),200)

)
HDU_INSTANTIATE_DATAUNIT(bt)
HDU_INSTANTIATE_DATAUNIT(all_types)

using namespace ::hatn::dataunit::types;

struct FieldName
{
    constexpr static const char* name="name";
};
template struct __declspec(dllexport) ::hatn::dataunit::Field<FieldName,TYPE_BYTES,17>;
using bytesT=::hatn::dataunit::Field<FieldName,TYPE_BYTES,17>;

bytesT fff()
{
    return bytesT();
}

struct Aaa
{
    bytesT ttt;
    Aaa():ttt(fff())
    {}

    Aaa(const bytesT& b):ttt(b)
    {}

    Aaa(bytesT&& b):ttt(std::move(b))
    {}
};

BOOST_AUTO_TEST_SUITE(Fieldss)

BOOST_AUTO_TEST_CASE(Performance)
{
    int runs=1000000;
    hatn::common::ElapsedTimer elapsed;

    auto perSecond=[&runs,&elapsed]()
    {
        auto ms=elapsed.elapsed().totalMilliseconds;
        if (ms==0)
        {
            return 1000*runs;
        }
        return static_cast<int>(round(1000*(runs/ms)));
    };

    //bytesT ttt1;
    //bytesT ttt2(ttt1);

    //Aaa aaa(fff());
    //bt::type t;
    all_types::type t;
    //auto ttt3=bytesT();
    //fff(ttt3);
    //bytesT ttt4(std::move(ttt1));

    std::cerr<<"Cycle creating unit"<<std::endl;

    size_t count=0;

    auto allocator=std::make_shared<hatn::common::pmr::polymorphic_allocator<char>>();

    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        all_types::type t;
        //uint64_t aaa=10;
        //std::tuple<uint64_t> t=std::make_tuple(10);
        //std::tuple<uint64_t> t=std::make_tuple(::hatn::common::ConstructWithArgsOrDefault<uint64_t,uint64_t>::f(10));
        //auto t=::hatn::common::ArgsListToAllTupleElements<uint64_t>::f(10);
        //count+=std::get<0>(t);

//        ::hatn::common::VInterfacesPack<uint64_t> t(::hatn::common::ArgsListToAllTupleElements<uint64_t>::f(10));
//        count+=t.getInterface<0>();
    }

    std::cerr<<"Duration "<<elapsed.toString(true)<<", perSecond="<<perSecond() << " count=" << count <<std::endl;
}


BOOST_AUTO_TEST_SUITE_END()

#if 0
HDU_DATAUNIT(all_types,
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
  HDU_FIELD(type_string,TYPE_STRING,12)
  HDU_FIELD(type_bytes,TYPE_BYTES,13)
  HDU_FIELD(type_fixed_string,HDU_TYPE_FIXED_STRING(8),20)
  HDU_FIELD_REQUIRED(type_int8_required,TYPE_INT8,25)
  HDU_ENUM(MyEnum,One=1,Two=2)
)
#endif

HATN_DATAUNIT_NAMESPACE_BEGIN

//        template struct __declspec(dllexport) Field<FieldName,TYPE_BOOL,1>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_INT8,2>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_INT16,3>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_INT32,4>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_INT64,5>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_UINT8,6>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_UINT16,7>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_UINT32,8>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_UINT64,9>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FIXED_INT32,10>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FIXED_INT64,11>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FIXED_UINT32,12>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FIXED_UINT64,13>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FLOAT,14>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_DOUBLE,15>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_STRING,16>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_BYTES,17>;
//        template struct __declspec(dllexport) Field<FieldName,TYPE_FIXED_STRING<8>,18>;

//        struct FieldName1
//        {
//            constexpr static const char* name="name1";
//        };
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_BOOL,1>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_INT8,2>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_INT16,3>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_INT32,4>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_INT64,5>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_UINT8,6>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_UINT16,7>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_UINT32,8>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_UINT64,9>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FIXED_INT32,10>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FIXED_INT64,11>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FIXED_UINT32,12>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FIXED_UINT64,13>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FLOAT,14>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_DOUBLE,15>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_STRING,16>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_BYTES,17>;
//        template struct __declspec(dllexport) Field<FieldName1,TYPE_FIXED_STRING<8>,18>;

//        struct FieldName2
//        {
//            constexpr static const char* name="name2";
//        };
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_BOOL,1>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_INT8,2>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_INT16,3>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_INT32,4>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_INT64,5>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_UINT8,6>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_UINT16,7>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_UINT32,8>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_UINT64,9>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FIXED_INT32,10>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FIXED_INT64,11>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FIXED_UINT32,12>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FIXED_UINT64,13>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FLOAT,14>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_DOUBLE,15>;

//        template struct __declspec(dllexport) Field<FieldName2,TYPE_STRING,16>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_BYTES,17>;
//        template struct __declspec(dllexport) Field<FieldName2,TYPE_FIXED_STRING<8>,18>;

//        struct Conf {constexpr static const char* name="unit";};
//        using unitType=UnitConcat<
//                    Conf,
//                    Field<FieldName,TYPE_BOOL,1>,
//                    Field<FieldName,TYPE_INT8,2>,
//                    Field<FieldName,TYPE_INT16,3>,
//                    Field<FieldName,TYPE_INT32,4>,
//                    Field<FieldName,TYPE_INT64,5>,
//                    Field<FieldName,TYPE_UINT8,6>,
//                    Field<FieldName,TYPE_UINT16,7>,
//                    Field<FieldName,TYPE_UINT32,8>,
//                    Field<FieldName,TYPE_UINT64,9>,
//                    Field<FieldName,TYPE_FIXED_INT32,10>,
//                    Field<FieldName,TYPE_FIXED_INT64,11>,
//                    Field<FieldName,TYPE_FIXED_UINT32,12>,
//                    Field<FieldName,TYPE_FIXED_UINT64,13>,
//                    Field<FieldName,TYPE_FLOAT,14>,
//                    Field<FieldName,TYPE_DOUBLE,15>,
//                    Field<FieldName,TYPE_STRING,16>,
//                    Field<FieldName,TYPE_BYTES,17>,
//                    Field<FieldName,TYPE_FIXED_STRING<8>,18>
//                >;
//        unitType unit;

//        using field1=Field<FieldName,TYPE_BOOL,1>;
//        using field2=Field<FieldName,TYPE_INT8,2>;
//        using field3=Field<FieldName,TYPE_INT16,3>;
//        using field4=Field<FieldName,TYPE_INT32,4>;
//        using field5=Field<FieldName,TYPE_INT64,5>;
//        using field6=Field<FieldName,TYPE_UINT8,6>;
//        using field7=Field<FieldName,TYPE_UINT16,7>;
//        using field8=Field<FieldName,TYPE_UINT32,8>;
//        using field9=Field<FieldName,TYPE_UINT64,9>;
//        using field10=Field<FieldName,TYPE_FIXED_INT32,10>;
//        using field11=Field<FieldName,TYPE_FIXED_INT64,11>;
//        using field12=Field<FieldName,TYPE_FIXED_UINT32,12>;
//        using field13=Field<FieldName,TYPE_FIXED_UINT64,13>;
//        using field14=Field<FieldName,TYPE_FLOAT,14>;
//        using field15=Field<FieldName,TYPE_DOUBLE,15>;
//        using field16=Field<FieldName2,TYPE_STRING,16>;
//        using field17=Field<FieldName2,TYPE_BYTES,17>;
//        using field18=Field<FieldName2,TYPE_FIXED_STRING<8>,18>;

//        struct Conf1 {constexpr static const char* name="unit1";};
//        template struct UnitConcat<
//                Conf1,
//                field1,
//                field2,
//                field3,
//                field4,
//                field5,
//                field6,
//                field7,
//                field8,
//                field9,
//                field10,
//                field11,
//                field12,
//                field13,
//                field14,
//                field15,
//                field16,
//                field17,
//                field18
//                >;

//        template struct __declspec(dllexport) RepeatedField<FieldName,TYPE_UINT32,1>;
//        template struct __declspec(dllexport) RepeatedField<FieldName1,TYPE_UINT64,1>;

//        struct Conf1 {constexpr static const char* name="unit1";};
//        using unitType1=UnitConcat<
//                    Conf1,
//                    Field<FieldName1,TYPE_BOOL,1>,
//                    Field<FieldName1,TYPE_INT8,2>,
//                    Field<FieldName1,TYPE_INT16,3>,
//                    Field<FieldName1,TYPE_INT32,4>,
//                    Field<FieldName1,TYPE_INT64,5>,
//                    Field<FieldName1,TYPE_UINT8,6>,
//                    Field<FieldName1,TYPE_UINT16,7>,
//                    Field<FieldName1,TYPE_UINT32,8>,
//                    Field<FieldName1,TYPE_UINT64,9>,
//                    Field<FieldName1,TYPE_FIXED_INT32,10>,
//                    Field<FieldName1,TYPE_FIXED_INT64,11>,
//                    Field<FieldName1,TYPE_FIXED_UINT32,12>,
//                    Field<FieldName1,TYPE_FIXED_UINT64,13>,
//                    Field<FieldName1,TYPE_FLOAT,14>,
//                    Field<FieldName1,TYPE_DOUBLE,15>,
//                    Field<FieldName1,TYPE_STRING,16>,
//                    Field<FieldName1,TYPE_BYTES,17>,
//                    Field<FieldName1,TYPE_FIXED_STRING<8>,18>
//                >;
//        unitType1 unit1;

//        struct Conf2 {constexpr static const char* name="unit2";};
//        using unitType2=UnitConcat<
//                    Conf2,
//                    Field<FieldName2,TYPE_BOOL,1>,
//                    Field<FieldName2,TYPE_INT8,2>,
//                    Field<FieldName2,TYPE_INT16,3>,
//                    Field<FieldName2,TYPE_INT32,4>,
//                    Field<FieldName2,TYPE_INT64,5>,
//                    Field<FieldName2,TYPE_UINT8,6>,
//                    Field<FieldName2,TYPE_UINT16,7>,
//                    Field<FieldName2,TYPE_UINT32,8>,
//                    Field<FieldName2,TYPE_UINT64,9>,
//                    Field<FieldName2,TYPE_FIXED_INT32,10>,
//                    Field<FieldName2,TYPE_FIXED_INT64,11>,
//                    Field<FieldName2,TYPE_FIXED_UINT32,12>,
//                    Field<FieldName2,TYPE_FIXED_UINT64,13>,
//                    Field<FieldName2,TYPE_FLOAT,14>,
//                    Field<FieldName2,TYPE_DOUBLE,15>,
//                    Field<FieldName2,TYPE_STRING,16>,
//                    Field<FieldName2,TYPE_BYTES,17>,
//                    Field<FieldName2,TYPE_FIXED_STRING<8>,18>
//                >;
//        unitType2 unit2;

    HATN_DATAUNIT_NAMESPACE_END
#endif
