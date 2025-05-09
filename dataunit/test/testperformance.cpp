#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <hatn/common/thread.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/makeshared.h>

#include <hatn/common/logger.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/elapsedtimer.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/dataunit/ipp/fieldserialization.ipp>
#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/unitmeta.ipp>
#include <hatn/dataunit/ipp/unittraits.ipp>

#include <hatn/test/multithreadfixture.h>

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {}

    ~Env()
    {}

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};

namespace internal {

HDU_V2_UNIT(all_types,
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
         HDU_V2_FIELD(type_string,TYPE_STRING,12)
         HDU_V2_FIELD(type_bytes,TYPE_BYTES,13)
         HDU_V2_FIELD(type_fixed_string,HDU_V2_TYPE_FIXED_STRING(8),20)
         HDU_V2_FIELD(type_int8_required,TYPE_INT8,25,true)
    )
}

namespace {

template <typename T> void fillForPerformance(T& allTypes, int n)
{
    auto& f1=allTypes.field(internal::all_types::type_bool);
    f1.set(true);

    int8_t val_int8=-10+n;
    auto& f2=allTypes.field(internal::all_types::type_int8);
    f2.set(val_int8);

    int8_t val_int8_required=123+n;
    auto& f2_2=allTypes.field(internal::all_types::type_int8_required);
    f2_2.set(val_int8_required);

    int16_t val_int16=0xF810+n;
    auto& f3=allTypes.field(internal::all_types::type_int16);
    f3.set(val_int16);

    int32_t val_int32=0x1F810110+n;
    auto& f4=allTypes.field(internal::all_types::type_int32);
    f4.set(val_int32);

    uint8_t val_uint8=10+n;
    auto& f5=allTypes.field(internal::all_types::type_uint8);
    f5.set(val_uint8);

    uint16_t val_uint16=0xF810+n;
    auto& f6=allTypes.field(internal::all_types::type_uint16);
    f6.set(val_uint16);

    uint32_t val_uint32=0x1F810110+n;
    auto& f7=allTypes.field(internal::all_types::type_uint32);
    f7.set(val_uint32);

    float val_float=253245.7686f+static_cast<float>(n);
    auto& f8=allTypes.field(internal::all_types::type_float);
    f8.set(val_float);

    float val_double=253245.7686f+static_cast<float>(n);
    auto& f9=allTypes.field(internal::all_types::type_double);
    f9.set(val_double);
}

HATN_COMMON_NAMESPACE::Result<size_t> checkIntResult(const std::string& msg, int i)
{
    if (i%10)
    return msg.size();

    return msg.size()+10;
}

}

BOOST_AUTO_TEST_SUITE(HduPerformance)

BOOST_FIXTURE_TEST_CASE(TestPerformance,Env,* boost::unit_test::disabled())
{
    int runs=10000000;
    HATN_COMMON_NAMESPACE::ElapsedTimer elapsed;
    uint64_t elapsedMs=0;

    auto perSecond=[&runs,&elapsedMs]()
    {
        auto ms=elapsedMs;
        if (ms==0)
        {
            return 1000*runs;
        }
        // codechecker_intentional [all] Don't care
        return static_cast<int>(round(1000*(runs/ms)));
    };

    std::cerr<<"Cycle result int"<<std::endl;

    std::string msg("Hello world!");
    size_t errcnt=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        errcnt+=checkIntResult(msg,i).value();
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    auto elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_GT(errcnt, runs);

    std::cerr<<"Cycle errors on stack"<<std::endl;
    errcnt=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        HATN_COMMON_NAMESPACE::Error ec;
        errcnt+=ec.message().size();
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(int(runs)*2,errcnt);

    std::cerr<<"Cycle creating DataUnit on stack"<<std::endl;

    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        typename internal::all_types::type unit1;
        auto& f1=unit1.field(internal::all_types::type_bool);
        f1.set(true);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;

    std::cerr<<"Cycle new/delete unit"<<std::endl;

    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        auto unit1=new typename internal::all_types::type();
        auto& f1=unit1->field(internal::all_types::type_bool);
        f1.set(true);
        delete unit1;
    }    
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;

    std::cerr<<"Cycle setting values"<<std::endl;

    typename internal::all_types::type unit1;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        fillForPerformance(unit1,i);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;

    std::cerr<<"Cycle serialization"<<std::endl;

    fillForPerformance(unit1,1234);
    hatn::dataunit::WireDataSingle wired;
    wired.mainContainer()->reserve(4096);
    int resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        resultCount+=static_cast<int>(hatn::dataunit::io::serialize(unit1,wired)>0);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    std::cerr<<"Cycle solid buf serialization"<<std::endl;

    fillForPerformance(unit1,1234);
    hatn::dataunit::WireBufSolid buf1;
    buf1.mainContainer()->reserve(4096);
    resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        resultCount+=static_cast<int>(hatn::dataunit::io::serialize(unit1,buf1)>0);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    std::cerr<<"Cycle polymorphic serialization"<<std::endl;

    fillForPerformance(unit1,1234);
    resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        resultCount+=(unit1.serialize(wired)>0);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    std::cerr<<"Cycle parsing"<<std::endl;

    hatn::dataunit::WireBufSolid solid1;
    BOOST_CHECK(hatn::dataunit::io::serialize(unit1,solid1)>0);

    typename internal::all_types::type unit2_1;
    resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        unit2_1.setClean(true);
        resultCount+=static_cast<int>(hatn::dataunit::io::deserialize(unit2_1,solid1));
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    std::cerr<<"Cycle polymorphic parsing"<<std::endl;

    hatn::dataunit::WireDataSingle wired1;
    unit1.serialize(wired1);

    typename internal::all_types::type unit2;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        unit2_1.setClean(true);
        unit2.parse(wired1);
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;

    std::cerr<<"Cycle create on stack and deserialize"<<std::endl;

    resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        typename internal::all_types::type unit2_1;
        resultCount+=static_cast<int>(hatn::dataunit::io::deserialize(unit2_1,solid1));
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    std::cerr<<"New/delete and deserialize"<<std::endl;

    resultCount=0;
    elapsed.reset();
    for (int i=0;i<runs;++i)
    {
        auto unit1=new typename internal::all_types::type();
        resultCount+=static_cast<int>(hatn::dataunit::io::deserialize(*unit1,solid1));
        delete unit1;
    }
    elapsedMs=elapsed.elapsed().totalMilliseconds;
    elapsedStr=elapsed.toString(true);
    std::cerr<<"Duration "<<elapsedStr<<", perSecond="<<perSecond()<<std::endl;
    BOOST_CHECK_EQUAL(runs,resultCount);

    //! @todo Test create in pool and deserialize.
}

BOOST_AUTO_TEST_CASE(TestPerformanceNoWarn)
{
    // to avoid warning about empty test tree
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
