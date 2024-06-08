#include <boost/test/unit_test.hpp>

#include <hatn/common/utils.h>
#include <hatn/common/fileutils.h>
#include <hatn/common/containerutils.h>

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/pmr/pmrtypes.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING

template <typename ContainerT>
void checkAssign()
{
    ContainerT b1("blablabla");
    size_t b1Size=b1.size();
    ContainerT b2=b1;
    BOOST_CHECK(b2==b1);
    b2.append("more bla");
    BOOST_CHECK(b2!=b1);
    BOOST_CHECK_EQUAL(b1.size(),b1Size);
    BOOST_CHECK_EQUAL(b1.c_str(),"blablabla");

    ContainerT b3("drop it");
    b3=b1;
    BOOST_CHECK(b3==b1);
    b3.append("more bla");
    BOOST_CHECK(b3!=b1);
    BOOST_CHECK_EQUAL(b1.size(),b1Size);
    BOOST_CHECK_EQUAL(b1.c_str(),"blablabla");

    b2=b1;
    b3=b1;

    ContainerT b4(std::move(b1));
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK(b4==b3);
    BOOST_CHECK(b1!=b3);
    b4.append("more bla");
    BOOST_CHECK(b4!=b3);
    BOOST_CHECK(b1!=b3);
    BOOST_CHECK_EQUAL(b1.size(),0);

    ContainerT b5("drop it");
    b5=std::move(b2);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(b2.size(),0);
    BOOST_CHECK(b5==b3);
    BOOST_CHECK(b2!=b3);
    b5.append("more bla");
    BOOST_CHECK(b5!=b2);
    BOOST_CHECK(b2!=b3);
    BOOST_CHECK_EQUAL(b2.size(),0);

    ContainerT b6("one");
    ContainerT b7("two");

    ContainerT b8;
    b8=b6+b7;
    BOOST_CHECK_EQUAL(b8.size(),b6.size()+b7.size());
    BOOST_CHECK_EQUAL(b8.c_str(),"onetwo");

    b8+=b8;
    BOOST_CHECK_EQUAL(b8.size(),2*(b6.size()+b7.size()));
    BOOST_CHECK_EQUAL(b8.c_str(),"onetwoonetwo");

    b8+="three";
    BOOST_CHECK_EQUAL(b8.c_str(),"onetwoonetwothree");

    b8=std::string("four")+std::string("five");
    BOOST_CHECK_EQUAL(b8.c_str(),"fourfive");

    b8=b8+ByteArray("six")+b8;
    BOOST_CHECK_EQUAL(b8.c_str(),"fourfivesixfourfive");
}

BOOST_AUTO_TEST_SUITE(TestByteArray)

template <typename T> void checkContainer(ByteArray& b, const T& initString, bool rawBuffer=false)
{
    BOOST_CHECK_EQUAL(b.size(),initString.size());
    BOOST_CHECK(strcmp(b.c_str(),initString.c_str())==0);
    BOOST_CHECK_EQUAL(b.isRawBuffer(),rawBuffer);
    BOOST_CHECK(!b.isEmpty());
    BOOST_CHECK(b.isEqual(initString.data(),initString.size()));
    BOOST_CHECK(b==initString);
    BOOST_CHECK_EQUAL(b.toStdString(),initString);
    bool ok=true;
    for (size_t i=0;i<b.size();i++)
    {
        ok=b[i]==initString.at(i);
        if (ok)
        {
            ok=*(b.data()+i)==initString.at(i);
        }
        if (ok)
        {
            ok=b.at(i)==initString.at(i);
        }
        if (!ok)
        {
            break;
        }
    }
    BOOST_CHECK(ok);

    auto capacity=b.capacity();
    b.clear();
    BOOST_CHECK(b.isEmpty());
    BOOST_CHECK_EQUAL(b.size(),0);
    BOOST_CHECK_EQUAL(b.capacity(),capacity);
    b.reset();
    BOOST_CHECK_EQUAL(b.capacity(),20/*ByteArray::PREALLOCATED_SIZE*/);
}

BOOST_AUTO_TEST_CASE(ByteArrayCtor)
{
/*
    ByteArray()=default;
*/
    ByteArray b1;
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK(!b1.isRawBuffer());
    BOOST_CHECK(b1.allocatorResource()==pmr::get_default_resource());

    auto allocator=common::pmr::AllocatorFactory::getDefault()->dataMemoryResource();
/*
    ByteArray(
        std::shared_ptr<pmr::polymorphic_allocator<char>> allocator//!< Allocator
    ) noexcept;
*/
    ByteArray b2(allocator);
    BOOST_CHECK(b2.isEmpty());
    BOOST_CHECK(!b2.isRawBuffer());
    b1.setAllocatorResource(b2.allocatorResource());
    BOOST_CHECK_EQUAL(allocator,b1.allocatorResource());
    BOOST_CHECK_EQUAL(allocator,b2.allocatorResource());

    auto doChecks=[&allocator](const std::string& initString)
    {
        /*
            //! Ctor from data buffer
            ByteArray(
                const char* data, //!< Data buffer to copy data from
                size_t size //!< Data size
            );
        */
        ByteArray b3(initString.data(),initString.size());
        checkContainer(b3,initString);

        /*
            //! Ctor from data buffer
            ByteArray(
                const char* data, //!< Data buffer to copy data from
                size_t size, //!< Data size
                std::shared_ptr<pmr::polymorphic_allocator<char>> allocator
            );
        */
        ByteArray b4(initString.data(),initString.size(),allocator);
        checkContainer(b4,initString);

        /*
            //! Ctor inline in raw data buffer
            ByteArray(
                const char* data, //!< Data buffer
                size_t size, //!< Data size
                bool inlineRawBuffer //!< Use raw buffer inline without allocating data
            );
        */
        ByteArray b5(initString.data(),initString.size(),false);
        checkContainer(b5,initString);

        /*
        //! Ctor inline in raw data buffer
        ByteArray(
            const char* data, //!< Data buffer
            size_t size, //!< Data size
            bool inlineRawBuffer, //!< Use raw buffer inline without allocating data
            std::shared_ptr<pmr::polymorphic_allocator<char>> allocator
        );
        */
        ByteArray b6(initString.data(),initString.size(),false,allocator);
        checkContainer(b6,initString);

        /*
        //! Ctor from null-terminated const char* string
        ByteArray(
            const char* data //!< Null-terminated char string
        );
        */
        ByteArray b7(initString.c_str());
        checkContainer(b7,initString);

        // copy ctor
        ByteArray b8(initString.c_str());
        ByteArray b9(b8);
        checkContainer(b8,initString);
        checkContainer(b9,initString);

        // copy assignment operator
        ByteArray b10(initString.c_str());
        ByteArray b11(allocator);
        b11=b10;
        checkContainer(b10,initString);
        checkContainer(b11,initString);

        // copy data
        ByteArray b12(initString.c_str());
        ByteArray b13(allocator);
        b13.copy(b12);
        checkContainer(b12,initString);
        checkContainer(b13,initString);

        // move ctor
        ByteArray b14(initString.c_str());
        BOOST_CHECK(!b14.isEmpty());
        ByteArray b15(std::move(b14));
        // codechecker_intentional [all]
        BOOST_CHECK(b14.isEmpty());
        checkContainer(b15,initString);

        // move assignment operator
        ByteArray b16(initString.c_str());
        BOOST_CHECK(!b16.isEmpty());
        ByteArray b17;
        BOOST_CHECK(b17.isEmpty());
        b17=std::move(b16);
        // codechecker_intentional [all]
        BOOST_CHECK(b16.isEmpty());
        checkContainer(b17,initString);
    };

    std::string fitPreallocatedStr="Hello";
    doChecks(fitPreallocatedStr);
    std::string notFitPreallocated="Hello world from hatn! This is rather long string tha does not fit into preallocated data array";
    doChecks(notFitPreallocated);
}

BOOST_AUTO_TEST_CASE(ByteArrayOp)
{
    auto doChecks=[](const std::string& initString)
    {
        ByteArray b;

        b.load(initString);
        checkContainer(b,initString);

        b.append(initString);
        checkContainer(b,initString);

        b.append(initString.c_str());
        checkContainer(b,initString);

        b.append(initString.data(),initString.size());
        checkContainer(b,initString);

        std::string extraFit="extra";
        std::string sFit=initString+extraFit;
        std::string sFitP=extraFit+initString;

        std::string extraNotFit="Extra extra data that will not fit into preallocated container";
        std::string sNoFit=initString+extraNotFit;
        std::string sNoFitP=extraNotFit+initString;

        std::string sChar=initString+'a';
        std::string sCharP='a'+initString;

        b.load(initString);
        b.append(extraFit);
        checkContainer(b,sFit);

        b.load(initString);
        b.append(extraFit.c_str());
        checkContainer(b,sFit);

        b.load(initString);
        b.append(extraNotFit);
        checkContainer(b,sNoFit);

        b.load(initString);
        b.append('a');
        checkContainer(b,sChar);

        b.load(initString);
        b.push_back('a');
        checkContainer(b,sChar);

        std::string sFitEp=initString+extraFit+'a'+'a'+extraFit;
        b.load(initString);
        b.append(extraFit);
        b.push_back('a');
        b.push_back('a');
        b.append(extraFit);
        checkContainer(b,sFitEp);

        std::string sNoFitEp=initString+extraNotFit+'a'+'a'+extraNotFit;
        b.load(initString);
        b.append(extraNotFit);
        b.push_back('a');
        b.push_back('a');
        b.append(extraNotFit);
        checkContainer(b,sNoFitEp);

        b.load(initString);
        b=b+extraFit;
        checkContainer(b,sFit);

        b.load(initString);
        b=b+extraNotFit;
        checkContainer(b,sNoFit);

        b.load(initString);
        b=b+'a';
        checkContainer(b,sChar);

        b.load(initString);
        b=b+extraFit.c_str();
        checkContainer(b,sFit);

        b.load(initString);
        b.prepend(extraFit);
        checkContainer(b,sFitP);

        b.load(initString);
        b.prepend(extraFit.c_str());
        checkContainer(b,sFitP);

        b.load(initString);
        b.prepend(extraNotFit);
        checkContainer(b,sNoFitP);

        b.load(initString);
        b.prepend('a');
        checkContainer(b,sCharP);

        b.load(initString);
        b.push_front('a');
        checkContainer(b,sCharP);

        std::string sFitPP=extraFit+'a'+'a'+extraFit+initString;
        b.load(initString);
        b.prepend(extraFit);
        b.push_front('a');
        b.push_front('a');
        b.prepend(extraFit);
        checkContainer(b,sFitPP);

        std::string sNoFitPP=extraNotFit+'a'+'a'+extraNotFit+initString;
        b.load(initString);
        b.prepend(extraNotFit);
        b.push_front('a');
        b.push_front('a');
        b.prepend(extraNotFit);
        checkContainer(b,sNoFitPP);
    };

    std::string fitPreallocatedStr="Hello";
    doChecks(fitPreallocatedStr);
    std::string notFitPreallocated="Hello world from hatn! This is rather long string tha does not fit into preallocated data array";
    doChecks(notFitPreallocated);
}

BOOST_AUTO_TEST_CASE(ByteArrayResize)
{
    std::array<char,100000> refArr;
    srand(static_cast<unsigned int>(time(NULL)));
    for (size_t i=0;i<refArr.size();i++)
    {
        refArr[i]=static_cast<char>(rand());
    }

    ByteArray b1;
    auto fillArray=[&refArr,&b1](size_t offset=0,size_t count=0)
    {
        if (count==0)
        {
            count=b1.size()-offset;
        }
        for (size_t i=offset;i<(offset+count);i++)
        {
            b1[i]=refArr[i];
        }
    };

    auto checkArray=[&refArr,&b1](size_t count, size_t offsetArr=0, size_t offsetRef=0)
    {
        BOOST_REQUIRE((count+offsetArr)<=b1.size());

        bool ok=true;
        for (size_t i=0;i<count;i++)
        {
            ok=b1[i+offsetArr]==refArr[i+offsetRef];
            if (!ok)
            {
                break;
            }
        }
        BOOST_CHECK(ok);
    };

    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),20);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.resize(5);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),20);
    BOOST_CHECK_EQUAL(b1.size(),5);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    fillArray();
    checkArray(5);

    b1.resize(100);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    checkArray(5);
    fillArray(5);
    checkArray(100);

    b1.resize(10);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),10);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    checkArray(10);

    b1.clear();
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.resize(1000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1000);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    fillArray();
    checkArray(1000);

    b1.reset();
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),20);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.resize(100000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),100000);
    BOOST_CHECK_EQUAL(b1.size(),100000);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    fillArray();
    checkArray(100000);

    b1.reset();
    b1.reserve(1000);
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.reserve(100);
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.shrink();
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),20);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.reserve(1000);
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);

    b1.resize(100);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    fillArray();
    checkArray(100);

    b1.shrink();
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28);
    checkArray(100);

    b1.rresize(1000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1000);
    BOOST_CHECK_EQUAL(b1.offset(),252);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28);
    checkArray(100,1000-100);
    fillArray(0,1000-100);
    checkArray(100,1000-100);
    checkArray(1000-100,0,0);

    b1.rresize(1280);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1280);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),0);
    checkArray(100,1280-100);
    checkArray(1000-100,1280-1000,0);
    fillArray(0,1280-1000);
    checkArray(100,1280-100);
    checkArray(1000-100,1280-1000,0);
    checkArray(1280-1000,0,0);

    b1.rresize(1268);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1268);
    BOOST_CHECK_EQUAL(b1.offset(),12);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),0);
    checkArray(100,1268-100);
    checkArray(1000-100,1268-1000,0);
    checkArray(1268-1000,0,12);

    b1.resize(1304);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1600);
    BOOST_CHECK_EQUAL(b1.size(),1304);
    BOOST_CHECK_EQUAL(b1.offset(),12);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),284);
    checkArray(100,1268-100);
    checkArray(1000-100,1268-1000,0);
    checkArray(1268-1000,0,12);
    fillArray(1300);
    checkArray(100,1268-100);
    checkArray(1000-100,1268-1000,0);
    checkArray(1268-1000,0,12);
    checkArray(4,1300,1300);

    b1.reset();
    b1.reserve(100);
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),128);

    b1.rresize(1000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1000);
    BOOST_CHECK_EQUAL(b1.offset(),152);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),128);
    fillArray();
    checkArray(1000);

    b1.resize(1008);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1008);
    BOOST_CHECK_EQUAL(b1.offset(),152);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),120);
    checkArray(1000);
    fillArray(1000);
    checkArray(8,1000,1000);
    checkArray(1008);

    b1.rresize(1016);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),1016);
    BOOST_CHECK_EQUAL(b1.offset(),144);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),120);
    checkArray(1008,1016-1008);
    fillArray(0,1016-1008);
    checkArray(1008,1016-1008);
    checkArray(1016-1008,0,0);

    b1.resize(516);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),516);
    BOOST_CHECK_EQUAL(b1.offset(),144);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),620);
    checkArray(516-(1016-1008),1016-1008);
    checkArray(1016-1008,0,0);

    b1.rresize(416);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),416);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),620);
    checkArray(416,0,516-416-(1016-1008));

    b1.reserve(1000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),416);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),620);
    checkArray(416,0,516-416-(1016-1008));

    b1.reserve(1000,true);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),1280);
    BOOST_CHECK_EQUAL(b1.size(),416);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),620);
    checkArray(416,0,516-416-(1016-1008));

    b1.reserve(500);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),832);
    BOOST_CHECK_EQUAL(b1.size(),416);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),172);
    checkArray(416,0,516-416-(1016-1008));

    b1.reserve(100,false);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),832);
    BOOST_CHECK_EQUAL(b1.size(),416);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),172);
    checkArray(416,0,516-416-(1016-1008));

    b1.resize(100);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),832);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),244);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),488);
    checkArray(100,0,516-416-(1016-1008));

    b1.shrink();
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28);
    checkArray(100,0,516-416-(1016-1008));

    b1.reserve(100000);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),100000);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),99900);
    checkArray(100,0,516-416-(1016-1008));

    b1.shrink();
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),128);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),0);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28);
    checkArray(100,0,516-416-(1016-1008));

    b1.reserve(100000,true);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),100000);
    BOOST_CHECK_EQUAL(b1.size(),100);
    BOOST_CHECK_EQUAL(b1.offset(),99872);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28);
    checkArray(100,0,516-416-(1016-1008));

    BOOST_CHECK_EQUAL(b1.left(200),100);
    checkArray(100,0,516-416-(1016-1008));

    BOOST_CHECK_EQUAL(b1.right(200),100);
    checkArray(100,0,516-416-(1016-1008));

    BOOST_CHECK_EQUAL(b1.left(86),86);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),100000);
    BOOST_CHECK_EQUAL(b1.size(),86);
    BOOST_CHECK_EQUAL(b1.offset(),99872);
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28+14);
    checkArray(86,0,516-416-(1016-1008));

    BOOST_CHECK_EQUAL(b1.right(50),50);
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.capacity(),100000);
    BOOST_CHECK_EQUAL(b1.size(),50);
    BOOST_CHECK_EQUAL(b1.offset(),99872+(86-50));
    BOOST_CHECK_EQUAL(b1.capacityForAppend(),28+14);
    checkArray(50,0,516-416-(1016-1008)+(86-50));
}

BOOST_AUTO_TEST_CASE(ByteArrayRawBuf)
{
    std::string str1="Hello world from hatn!";
    ByteArray b1(str1.data(),str1.size(),true);

    BOOST_CHECK(b1.isRawBuffer());
    BOOST_CHECK(!b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.size(),str1.size());
    BOOST_CHECK_EQUAL(b1.capacity(),str1.size());
    BOOST_CHECK_EQUAL(b1.data(),str1.data());
    BOOST_CHECK(strcmp(b1.c_str(),str1.c_str())==0);

    auto b2=ByteArray::fromRawData(b1.data(),b1.size());
    BOOST_CHECK(b2.isRawBuffer());
    BOOST_CHECK(!b2.isEmpty());
    BOOST_CHECK_EQUAL(b2.size(),str1.size());
    BOOST_CHECK_EQUAL(b2.capacity(),str1.size());
    BOOST_CHECK_EQUAL(b2.data(),str1.data());
    BOOST_CHECK(strcmp(b2.c_str(),str1.c_str())==0);

    b2.reset();
    BOOST_CHECK(!b2.isRawBuffer());
    BOOST_CHECK(b2.isEmpty());
    BOOST_CHECK_EQUAL(b2.size(),0);
    BOOST_CHECK_EQUAL(b2.capacity(),20);
}

BOOST_AUTO_TEST_CASE(ByteArrayFill)
{
    size_t size=100;

    ByteArray b1;
    b1.resize(size);
    b1.fill('a');
    BOOST_CHECK_EQUAL(b1.size(),size);

    bool ok=true;
    for (size_t i=0;i<size;i++)
    {
        ok=b1[i]=='a';
        if (!ok)
        {
            break;
        }
    }
    BOOST_CHECK(ok);

    b1.fill('b',10);
    BOOST_CHECK_EQUAL(b1.size(),size);

    ok=true;
    for (size_t i=0;i<size;i++)
    {
        if (i<10)
        {
            ok=b1[i]=='a';
        }
        else
        {
            ok=b1[i]=='b';
        }
        if (!ok)
        {
            break;
        }
    }
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(ByteArrayFromFile)
{
    ByteArray b1;
    std::string fileName=fmt::format("{}/common/assets/testbytearray.txt",test::MultiThreadFixture::assetsPath());
    BOOST_TEST_MESSAGE(fileName);

    auto ec=b1.loadFromFile(fileName);
    BOOST_REQUIRE(ec.isNull());
    std::string loaded(b1.c_str());

    std::string sample="Test loading of common::ByteArray's content from file";
    BOOST_CHECK_EQUAL(loaded,sample);

    BOOST_CHECK(!b1.loadFromFile("notexists.txt").isNull());
}

BOOST_AUTO_TEST_CASE(ByteArrayToFile)
{
    ByteArray b1("Hello from hatn!\nThree lines\nThanks.");
    std::string fileName=fmt::format("{}/writebytearray{}.txt",test::MultiThreadFixture::tmpPath(),Random::generate(10000));
    BOOST_TEST_MESSAGE(fileName);
    auto ec=b1.saveToFile(fileName);
    BOOST_REQUIRE(ec.isNull());

    ByteArray b2;
    ec=b2.loadFromFile(fileName);
    BOOST_REQUIRE(ec.isNull());

    BOOST_CHECK(b1==b2);

    ec=FileUtils::remove(fileName);
    BOOST_CHECK(!ec);
}

BOOST_AUTO_TEST_CASE(ByteArrayHex)
{
    std::string hex="0123456789ABCDEF";
    ByteArray b1;
    ContainerUtils::hexToRaw(hex,b1);

    std::vector<uint8_t> raw({0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF});
    BOOST_CHECK(b1.isEqual(raw.data(),raw.size()));

    ByteArray b2;
    ContainerUtils::rawToHex(raw,b2);

    BOOST_CHECK(b2.isEqual(hex.data(),hex.size()));
}

BOOST_AUTO_TEST_CASE(ByteArrayBase64)
{
    std::string str="Hello from hatn!";
    std::string b64Str="SGVsbG8gZnJvbSBoYXRuIQ==";

    ByteArray b1;
    ContainerUtils::rawToBase64(str,b1);
    BOOST_CHECK(b1.isEqual(b64Str.data(),b64Str.size()));

    ByteArray b2;
    ContainerUtils::base64ToRaw(b64Str,b2);
    BOOST_CHECK(b2.isEqual(str.data(),str.size()));
}

BOOST_AUTO_TEST_CASE(ByteArrayAssign)
{
    checkAssign<ByteArray>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestFixedByteArray)

BOOST_AUTO_TEST_CASE(FixedByteArrayAssign)
{
    checkAssign<FixedByteArray128>();
}

BOOST_AUTO_TEST_SUITE_END()
