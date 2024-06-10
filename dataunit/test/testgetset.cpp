#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <hatn/common/thread.h>
#include <hatn/common/bytearray.h>

#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/test/multithreadfixture.h>
#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

//#define HATN_TEST_LOG_CONSOLE

HATN_COMMON_USING
HATN_DATAUNIT_USING

namespace {

static void setLogHandler()
{
    auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #else
            std::ignore=s;
        #endif
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
    ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::DEBUG);
    ::hatn::common::Logger::setDefaultDebugLevel(1);
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            setLogHandler();
            ::hatn::common::Logger::setFatalTracing(false);
            ::hatn::common::Logger::start(false);
        }
    }

    ~Env()
    {
        ::hatn::common::Logger::stop();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};

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

    int8_t   int8ValSampleNeg=-10;
    int16_t  int16ValSampleNeg=-10000;
    int32_t  int32ValSampleNeg=-1000000;
    int64_t  int64ValSampleNeg=-10000000000;

    int8_t   int8ValSamplePos=10;
    int16_t  int16ValSamplePos=10000;
    int32_t  int32ValSamplePos=10000000;
    int64_t  int64ValSamplePos=10000000000;
    uint8_t  uint8ValSamplePos=10;
    uint16_t uint16ValSamplePos=10000;
    uint32_t uint32ValSamplePos=1000000;
    uint64_t uint64ValSamplePos=10000000000;
}

BOOST_AUTO_TEST_SUITE(TestGetSet)

template <typename T, typename FieldT, typename ObjT>
void checkScalarField(ObjT* obj,
                const T& valPos,
                const std::array<T,3>& checkValsPos,
                const std::array<bool,15>& checkValsPosResults,
                const T& valNeg=T(),
                const std::array<T,3>& checkValsNeg=std::array<T,3>(),
                const std::array<bool,15>& checkValsNegResults=std::array<bool,15>()
                )
{
    T valSample=0;

    auto field=obj->fieldById(FieldT::id());
    T val=valPos;
    valSample=checkValsPos[0];
    field->setValue(val);
    BOOST_CHECK(field->equals(val));
    BOOST_CHECK(field->less(valSample)==checkValsPosResults[0]);
    valSample=checkValsPos[1];
    BOOST_CHECK(field->less(valSample)==checkValsPosResults[1]);
    valSample=checkValsPos[2];
    BOOST_CHECK(field->less(valSample)==checkValsPosResults[2]);
    valSample=0;
    field->getValue(valSample);
    BOOST_CHECK_EQUAL(val,valSample);

    BOOST_CHECK(field->less(int8ValSamplePos)==checkValsPosResults[3]);
    BOOST_CHECK(field->less(int16ValSamplePos)==checkValsPosResults[4]);
    BOOST_CHECK(field->less(int32ValSamplePos)==checkValsPosResults[5]);
    BOOST_CHECK(field->less(int64ValSamplePos)==checkValsPosResults[6]);
    BOOST_CHECK(field->less(uint8ValSamplePos)==checkValsPosResults[7]);
    BOOST_CHECK(field->less(uint16ValSamplePos)==checkValsPosResults[8]);
    BOOST_CHECK(field->less(uint32ValSamplePos)==checkValsPosResults[9]);
    BOOST_CHECK(field->less(uint64ValSamplePos)==checkValsPosResults[10]);
    BOOST_CHECK(field->less(int8ValSampleNeg)==checkValsPosResults[11]);
    BOOST_CHECK(field->less(int16ValSampleNeg)==checkValsPosResults[12]);
    BOOST_CHECK(field->less(int32ValSampleNeg)==checkValsPosResults[13]);
    BOOST_CHECK(field->less(int64ValSampleNeg)==checkValsPosResults[14]);

    if (std::is_signed<T>::value)
    {
        val=valNeg;
        valSample=checkValsNeg[0];
        field->setValue(val);
        BOOST_CHECK(field->equals(val));
        BOOST_CHECK(field->less(valSample)==checkValsNegResults[0]);
        valSample=checkValsNeg[1];
        BOOST_CHECK(field->less(valSample)==checkValsNegResults[1]);
        valSample=checkValsNeg[2];
        BOOST_CHECK(field->less(valSample)==checkValsNegResults[2]);
        valSample=0;
        field->getValue(valSample);
        BOOST_CHECK_EQUAL(val,valSample);

        BOOST_CHECK(field->less(int8ValSamplePos)==checkValsNegResults[3]);
        BOOST_CHECK(field->less(int16ValSamplePos)==checkValsNegResults[4]);
        BOOST_CHECK(field->less(int32ValSamplePos)==checkValsNegResults[5]);
        BOOST_CHECK(field->less(int64ValSamplePos)==checkValsNegResults[6]);
        BOOST_CHECK(field->less(uint8ValSamplePos)==checkValsNegResults[7]);
        BOOST_CHECK(field->less(uint16ValSamplePos)==checkValsNegResults[8]);
        BOOST_CHECK(field->less(uint32ValSamplePos)==checkValsNegResults[9]);
        BOOST_CHECK(field->less(uint64ValSamplePos)==checkValsNegResults[10]);
        BOOST_CHECK(field->less(int8ValSampleNeg)==checkValsNegResults[11]);
        BOOST_CHECK(field->less(int16ValSampleNeg)==checkValsNegResults[12]);
        BOOST_CHECK(field->less(int32ValSampleNeg)==checkValsNegResults[13]);
        BOOST_CHECK(field->less(int64ValSampleNeg)==checkValsNegResults[14]);
    }
}

template <typename T>
void checkScalarTypes(T* obj)
{
    BOOST_CHECK_EQUAL(scalar_types::type_bool.id(),1);
    BOOST_CHECK_EQUAL(scalar_types::type_int8.id(),2);
    BOOST_CHECK_EQUAL(scalar_types::type_int16.id(),3);
    BOOST_CHECK_EQUAL(scalar_types::type_int32.id(),4);
    BOOST_CHECK_EQUAL(scalar_types::type_int64.id(),5);
    BOOST_CHECK_EQUAL(scalar_types::type_uint8.id(),6);
    BOOST_CHECK_EQUAL(scalar_types::type_uint16.id(),7);
    BOOST_CHECK_EQUAL(scalar_types::type_uint32.id(),8);
    BOOST_CHECK_EQUAL(scalar_types::type_uint64.id(),9);
    BOOST_CHECK_EQUAL(scalar_types::type_float.id(),10);
    BOOST_CHECK_EQUAL(scalar_types::type_double.id(),11);
    BOOST_CHECK_EQUAL(scalar_types::type_enum.id(),12);

    auto boolField=obj->fieldById(scalar_types::type_bool.id());
    boolField->setValue(true);
    BOOST_CHECK(boolField->equals(true));
    BOOST_CHECK(!boolField->less(false));
    bool boolVal=false;
    boolField->getValue(boolVal);
    BOOST_CHECK_EQUAL(boolVal,true);

    BOOST_TEST_CONTEXT("int8_t")
    {
        checkScalarField<int8_t,decltype(scalar_types::type_int8)>(
            obj,
            int8_t(11),
            {int8_t(12),int8_t(9),int8_t(-9)},
            {true,false,false,
             false,true,true,true,false,true,true,true,false,false,false,false
            },
            int8_t(-11),
            {int8_t(-12),int8_t(9),int8_t(-9)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,false,false,false
            }
        );
    }

    BOOST_TEST_CONTEXT("int16_t")
    {
        checkScalarField<int16_t,decltype(scalar_types::type_int16)>(
            obj,
            int16_t(11000),
            {int16_t(12000),int16_t(9000),int16_t(-9000)},
            {true,false,false,
             false,false,true,true,false,false,true,true,false,false,false,false
            },
            int16_t(-11000),
            {int16_t(-12000),int16_t(9000),int16_t(-9000)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,true,false,false
            }
        );
    }
    BOOST_TEST_CONTEXT("int32_t")
    {
        checkScalarField<int32_t,decltype(scalar_types::type_int32)>(
            obj,
            int32_t(1100000),
            {int32_t(1200000),int32_t(900000),int32_t(-900000)},
            {true,false,false,
             false,false,true,true,false,false,false,true,false,false,false,false
            },
            int32_t(-1100000),
            {int32_t(-1200000),int32_t(900000),int32_t(-900000)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,true,true,false
            }
        );
    }
    BOOST_TEST_CONTEXT("int64_t")
    {
        checkScalarField<int64_t,decltype(scalar_types::type_int64)>(
            obj,
            int64_t(11000000000),
            {int64_t(12000000000),int64_t(9000000000),int64_t(-9000000000)},
            {true,false,false,
             false,false,false,false,false,false,false,false,false,false,false,false
            },
            int64_t(-11000000000),
            {int64_t(-12000000000),int64_t(9000000000),int64_t(-9000000000)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,true,true,true
            }
        );
    }

    BOOST_TEST_CONTEXT("uint8_t")
    {
        checkScalarField<uint8_t,decltype(scalar_types::type_uint8)>(
            obj,
            uint8_t(11),
            {uint8_t(12),uint8_t(9),uint8_t(0)},
            {true,false,false,
             false,true,true,true,false,true,true,true,false,false,false,false
            }
        );
    }
    BOOST_TEST_CONTEXT("uint16_t")
    {
        checkScalarField<uint16_t,decltype(scalar_types::type_uint16)>(
            obj,
            uint16_t(11000),
            {int16_t(12000),uint16_t(9000),uint16_t(0)},
            {true,false,false,
             false,false,true,true,false,false,true,true,false,false,false,false
            }
        );
    }
    BOOST_TEST_CONTEXT("uint32_t")
    {
        checkScalarField<uint32_t,decltype(scalar_types::type_uint32)>(
            obj,
            uint32_t(1100000),
            {uint32_t(1200000),uint32_t(900000),uint32_t(0)},
            {true,false,false,
             false,false,true,true,false,false,false,true,false,false,false,false
            }
        );
    }
    BOOST_TEST_CONTEXT("uint64_t")
    {
        checkScalarField<uint64_t,decltype(scalar_types::type_uint64)>(
            obj,
            uint64_t(11000000000),
            {uint64_t(12000000000),uint64_t(9000000000),uint64_t(0)},
            {true,false,false,
             false,false,false,false,false,false,false,false,false,false,false,false
            }
        );
    }

    BOOST_TEST_CONTEXT("float")
    {
        checkScalarField<float,decltype(scalar_types::type_float)>(
            obj,
            float(11),
            {float(12),float(9),float(-9)},
            {true,false,false,
             false,true,true,true,false,true,true,true,false,false,false,false
            },
            float(-11),
            {float(-12),float(9),float(-9)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,false,false,false
            }
        );
    }
    BOOST_TEST_CONTEXT("double")
    {
        checkScalarField<double,decltype(scalar_types::type_double)>(
            obj,
            double(11000000000),
            {double(12000000000),double(9000000000),double(-9000000000)},
            {true,false,false,
             false,false,false,false,false,false,false,false,false,false,false,false
            },
            double(-11000000000),
            {double(-12000000000),double(9000000000),double(-9000000000)},
            {false,true,true,
             true,true,true,true,true,true,true,true,true,true,true,true
            }
        );
    }
}

BOOST_FIXTURE_TEST_CASE(TestScalar,Env)
{
    using traits=scalar_types::traits;
    using type=typename traits::type;
    type obj;
    checkScalarTypes(&obj);
}

template <typename FieldT, typename ObjT>
void checkByteField(ObjT* obj, bool shared)
{
    auto field=obj->fieldById(FieldT::id());
    if (shared)
    {
        field->bufCreateShared();
    }

    BOOST_CHECK(field->bufEmpty());
    if (!std::is_same<FieldT,decltype(byte_types::type_fixed_string)>::value)
    {
        field->bufReserve(10000u);
        field->bufResize(1000u);
        BOOST_CHECK_EQUAL(field->bufSize(),1000u);
        BOOST_CHECK_GE(field->bufCapacity(),10000u);
    }
    else
    {
        field->bufReserve(100u);
        field->bufResize(10u);
        BOOST_CHECK_EQUAL(field->bufSize(),10u);
        BOOST_CHECK_GE(field->bufCapacity(),100u);
    }
    BOOST_CHECK(!field->bufEmpty());

    ByteArray sample1("hello world from Hatn");
    ByteArray sample2("another text");
    ByteArray sample3("Hello world from Hatn");
    ByteArray sample4("Zaza text");

    field->setValue(sample1);

    BOOST_CHECK(field->equals(sample1.data(),sample1.size()));
    BOOST_CHECK(field->lexEquals(sample1.data(),sample1.size()));

    BOOST_CHECK(!field->equals(sample3.data(),sample3.size()));
    BOOST_CHECK(!field->lexEquals(sample3.data(),sample3.size()));
    BOOST_CHECK(field->lexEqualsI(sample3.data(),sample3.size()));
    BOOST_CHECK(!field->lexLess(sample3.data(),sample3.size()));

    BOOST_CHECK(!field->equals(sample2.data(),sample2.size()));
    BOOST_CHECK(!field->lexEquals(sample2.data(),sample3.size()));
    BOOST_CHECK(!field->lexEqualsI(sample2.data(),sample3.size()));
    BOOST_CHECK(!field->less(sample2.data(),sample2.size()));
    BOOST_CHECK(!field->lexLess(sample2.data(),sample2.size()));
    BOOST_CHECK(!field->lexLessI(sample2.data(),sample2.size()));

    BOOST_CHECK(!field->lexLess(sample4.data(),sample4.size()));
    BOOST_CHECK(field->lexLessI(sample4.data(),sample4.size()));

    BOOST_CHECK_EQUAL(field->bufSize(),sample1.size());
    BOOST_CHECK(sample1.isEqual(field->bufData(),field->bufSize()));
    BOOST_CHECK(sample1.isEqual(field->bufCStr(),field->bufSize()));
}

template <typename T>
void checkByteTypes(T* obj, bool shared)
{
    BOOST_TEST_CONTEXT("bytes"){checkByteField<decltype(byte_types::type_bytes)>(obj,shared);}
    BOOST_TEST_CONTEXT("string"){checkByteField<decltype(byte_types::type_string)>(obj,shared);}
    BOOST_TEST_CONTEXT("fixed_string"){checkByteField<decltype(byte_types::type_fixed_string)>(obj,shared);}
}

BOOST_FIXTURE_TEST_CASE(TestBytes,Env)
{
    using traits=byte_types::traits;
    using type=typename traits::type;

    type obj1, obj2;

    checkByteTypes(&obj1,false);
    checkByteTypes(&obj2,false);
}

BOOST_FIXTURE_TEST_CASE(TestSubunit,Env)
{
    using traits=subunit_types::traits;
    using type=typename traits::type;
    using shared_raits=subunit_types::shared_traits;
    using shared_type=typename shared_raits::type;

    type obj1;
    shared_type obj2;

    auto scalarUnitField1=obj1.fieldById(subunit_types::scalar.id());
    auto* scalarSubunit1=scalarUnitField1->subunit();
    checkScalarTypes(scalarSubunit1);
    auto byteUnitField1=obj1.fieldById(subunit_types::bytes.id());
    auto* byteSubunit1=byteUnitField1->subunit();
    checkByteTypes(byteSubunit1,false);

    auto scalarUnitField2=obj2.fieldById(subunit_types::scalar.id());
    auto* scalarSubunit2=scalarUnitField2->subunit();
    checkScalarTypes(scalarSubunit2);
    auto byteUnitField2=obj2.fieldById(subunit_types::bytes.id());
    auto* byteSubunit2=byteUnitField2->subunit();
    checkByteTypes(byteSubunit2,true);
}

template <typename ObjT, typename FieldT, typename T>
void checkScalarArray(
        const std::vector<T>& vec
    )
{
    ObjT obj1;

    auto field1=obj1.fieldById(FieldT::id());
    BOOST_REQUIRE(field1);

    BOOST_CHECK(field1->arrayEmpty());
    BOOST_CHECK_EQUAL(field1->arrayCapacity(),0);
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    field1->arrayReserve(vec.capacity());
    BOOST_CHECK_EQUAL(field1->arrayCapacity(),vec.capacity());
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    field1->arrayResize(vec.size());
    BOOST_CHECK_EQUAL(field1->arraySize(),vec.size());
    BOOST_CHECK(!field1->arrayEmpty());

    for (size_t i=0;i<vec.size();i++)
    {
        field1->arraySet(i,vec[i]);
    }
    for (size_t i=0;i<vec.size();i++)
    {
        T val=static_cast<T>(0);
        field1->arrayGet(i,val);
        BOOST_CHECK_EQUAL(val,vec[i]);
    }

    field1->arrayClear();
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    BOOST_CHECK(field1->arrayEmpty());

    for (size_t i=0;i<vec.size();i++)
    {
        field1->arrayAdd(vec[i]);
    }
    BOOST_CHECK_EQUAL(field1->arraySize(),vec.size());
    BOOST_CHECK(!field1->arrayEmpty());

    for (size_t i=0;i<vec.size();i++)
    {
        T val=static_cast<T>(0);
        field1->arrayGet(i,val);
        BOOST_CHECK_EQUAL(val,vec[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(TestScalarArrays,Env)
{
    using traits=scalar_arrays::traits;
    using type=typename traits::type;

    BOOST_TEST_CONTEXT("bool"){checkScalarArray<type,decltype(scalar_arrays::type_int8),bool>({true,true,false,true,false,false});}
    BOOST_TEST_CONTEXT("int8"){checkScalarArray<type,decltype(scalar_arrays::type_int8),int8_t>({int8_t(10),int8_t(-10),int8_t(0),int8_t(120),int8_t(-30),int8_t(50)});}
    BOOST_TEST_CONTEXT("int16"){checkScalarArray<type,decltype(scalar_arrays::type_int16),int16_t>({int16_t(1000),int16_t(-100),int16_t(0),int16_t(12000),int16_t(-300),int16_t(500)});}
    BOOST_TEST_CONTEXT("int32"){checkScalarArray<type,decltype(scalar_arrays::type_int32),int32_t>({int32_t(100000),int32_t(-100000),int32_t(0),int32_t(12000000),int32_t(-300000),int32_t(500000)});}
    BOOST_TEST_CONTEXT("int64"){checkScalarArray<type,decltype(scalar_arrays::type_int64),int64_t>({int64_t(10000000000),int64_t(-10000000000),int64_t(0),int64_t(12000000000),int64_t(-300000000000),int64_t(500000000000)});}
    BOOST_TEST_CONTEXT("uint8"){checkScalarArray<type,decltype(scalar_arrays::type_uint8),uint8_t>({uint8_t(10),uint8_t(0),uint8_t(120),uint8_t(50)});}
    BOOST_TEST_CONTEXT("uint16"){checkScalarArray<type,decltype(scalar_arrays::type_uint16),uint16_t>({uint16_t(1000),uint16_t(0),uint16_t(12000),uint16_t(500)});}
    BOOST_TEST_CONTEXT("uint32"){checkScalarArray<type,decltype(scalar_arrays::type_uint32),uint32_t>({uint32_t(100000),uint32_t(0),uint32_t(12000000),uint32_t(500000)});}
    BOOST_TEST_CONTEXT("uint64"){checkScalarArray<type,decltype(scalar_arrays::type_uint64),uint64_t>({uint64_t(10000000000),uint64_t(0),uint64_t(12000000000),uint64_t(500000000000)});}

    BOOST_TEST_CONTEXT("float"){checkScalarArray<type,decltype(scalar_arrays::type_float),float>({float(100000),float(-100000),float(0),float(12000000),float(-300000),float(500000)});}
    BOOST_TEST_CONTEXT("double"){checkScalarArray<type,decltype(scalar_arrays::type_double),double>({double(10000000000),double(-10000000000),double(0),double(12000000000),double(-300000000000),double(500000000000)});}
}

template <typename ObjT, typename FieldT>
void checkByteArray(bool shared)
{
    ObjT obj1;

    std::vector<ByteArray> vec={"one","two","three","four","five"};

    auto field1=obj1.fieldById(FieldT::id());
    BOOST_REQUIRE(field1);
    field1->setParseToSharedArrays(shared);

    BOOST_CHECK(field1->arrayEmpty());
    BOOST_CHECK_EQUAL(field1->arrayCapacity(),0);
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    field1->arrayReserve(vec.capacity());
    BOOST_CHECK_EQUAL(field1->arrayCapacity(),vec.capacity());
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    field1->arrayResize(vec.size());
    BOOST_CHECK_EQUAL(field1->arraySize(),vec.size());
    BOOST_CHECK(!field1->arrayEmpty());

    for (size_t i=0;i<vec.size();i++)
    {
        if (shared)
        {
            field1->arrayBufCreateShared(i);
        }

        BOOST_CHECK(field1->arrayBufEmpty(i));
        BOOST_CHECK_EQUAL(field1->arrayBufSize(i),0);

        field1->arrayBufReserve(i,100u);
        BOOST_CHECK(field1->arrayBufEmpty(i));
        BOOST_CHECK_GE(field1->arrayBufCapacity(i),100u);
        BOOST_CHECK_EQUAL(field1->arrayBufSize(i),0);

        field1->arrayBufResize(i,10u);
        BOOST_CHECK(!field1->arrayBufEmpty(i));
        BOOST_CHECK_GE(field1->arrayBufCapacity(i),100u);
        BOOST_CHECK_EQUAL(field1->arrayBufSize(i),10u);

        field1->arrayBufClear(i);
        BOOST_CHECK(field1->arrayBufEmpty(i));
        BOOST_CHECK_GE(field1->arrayBufCapacity(i),100u);
        BOOST_CHECK_EQUAL(field1->arrayBufSize(i),0);

        field1->arrayBufSetValue(i,vec[i]);
    }
    for (size_t i=0;i<vec.size();i++)
    {
        BOOST_CHECK(vec[i].isEqual(field1->arrayBufData(i),field1->arrayBufSize(i)));
        BOOST_CHECK(vec[i].isEqual(field1->arrayBufCStr(i)));
    }

    field1->arrayClear();
    BOOST_CHECK_EQUAL(field1->arraySize(),0);
    BOOST_CHECK(field1->arrayEmpty());

    ObjT obj2;
    auto field2=obj2.fieldById(FieldT::id());
    BOOST_REQUIRE(field2);
    field2->setParseToSharedArrays(shared);

    for (size_t i=0;i<vec.size();i++)
    {
        field2->arrayBufAddValue(vec[i]);
    }
    BOOST_CHECK_EQUAL(field2->arraySize(),vec.size());
    BOOST_CHECK(!field2->arrayEmpty());

    for (size_t i=0;i<vec.size();i++)
    {
        BOOST_CHECK(vec[i].isEqual(field2->arrayBufData(i),field2->arrayBufSize(i)));
        BOOST_CHECK(vec[i].isEqual(field2->arrayBufCStr(i)));
    }
}

static void checkByteArrays(bool shared)
{
    using traits=buf_arrays::traits;
    using type=typename traits::type;

    BOOST_TEST_CONTEXT("bytes"){checkByteArray<type,decltype(buf_arrays::type_bytes)>(shared);}
    BOOST_TEST_CONTEXT("string"){checkByteArray<type,decltype(buf_arrays::type_string)>(shared);}
    BOOST_TEST_CONTEXT("fixed_string"){checkByteArray<type,decltype(buf_arrays::type_fixed_string)>(shared);}
}

BOOST_FIXTURE_TEST_CASE(TestBufArrays,Env)
{
    BOOST_TEST_CONTEXT("onstack"){checkByteArrays(false);}
    BOOST_TEST_CONTEXT("shared"){checkByteArrays(true);}
}

template <typename ObjT>
void checkSubunitArray()
{
    ObjT obj;
    auto scalarUnitArray=obj.fieldById(subunit_arrays::scalar.id());

    std::vector<Unit*> vec(5);

    BOOST_CHECK(scalarUnitArray->arrayEmpty());
    BOOST_CHECK_EQUAL(scalarUnitArray->arrayCapacity(),0);
    BOOST_CHECK_EQUAL(scalarUnitArray->arraySize(),0);
    scalarUnitArray->arrayReserve(vec.capacity());
    BOOST_CHECK_EQUAL(scalarUnitArray->arrayCapacity(),vec.capacity());
    BOOST_CHECK_EQUAL(scalarUnitArray->arraySize(),0);
    scalarUnitArray->arrayResize(vec.size());
    BOOST_CHECK_EQUAL(scalarUnitArray->arraySize(),vec.size());
    BOOST_CHECK(!scalarUnitArray->arrayEmpty());

    auto scalarUnit3=scalarUnitArray->arraySubunit(3);
    checkScalarTypes(scalarUnit3);

    scalarUnitArray->arrayClear();
    BOOST_CHECK_EQUAL(scalarUnitArray->arraySize(),0);
    BOOST_CHECK(scalarUnitArray->arrayEmpty());
}

BOOST_FIXTURE_TEST_CASE(TestSubunitArrays,Env)
{
    using traits=subunit_arrays::traits;
    using type=typename traits::type;
    using shared_traits=subunit_arrays::shared_traits;
    using shared_type=typename shared_traits::type;

    checkSubunitArray<type>();
    checkSubunitArray<shared_type>();
}

BOOST_AUTO_TEST_SUITE_END()
