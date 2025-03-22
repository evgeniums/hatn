#include <iostream>
#include <boost/test/unit_test.hpp>

#include <hatn/common/fixedbytearray.h>
#include <hatn/common/objectid.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestFixedByteArray)

template <typename T> static void checkArray(const std::string& str, const T& fb)
{
    BOOST_CHECK(!fb.isEmpty());
    BOOST_CHECK_EQUAL(fb.size(),str.size());
    BOOST_CHECK(fb.isEqual(str.data(),str.size()));
    BOOST_CHECK(fb==str);
    BOOST_CHECK_EQUAL(fb.toStdString(),str);
    BOOST_CHECK_THROW(fb.at(fb.size()+1),std::out_of_range);

    bool ok=true;
    for (size_t i=0;i<fb.size();i++)
    {
        ok=fb[i]==str.at(i);
        if (ok)
        {
            ok=*(fb.data()+i)==str.at(i);
        }
        if (ok)
        {
            ok=fb.at(i)==str.at(i);
        }
        if (!ok)
        {
            break;
        }
    }
    BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(FixedByteArrayCtor)
{
    FixedByteArray8 fb8;
    BOOST_CHECK_EQUAL(fb8.capacity(),8);
    FixedByteArray16 fb16;
    BOOST_CHECK_EQUAL(fb16.capacity(),16);
    FixedByteArray20 fb20;
    BOOST_CHECK_EQUAL(fb20.capacity(),20);
    FixedByteArray32 fb32;
    BOOST_CHECK_EQUAL(fb32.capacity(),32);
    FixedByteArray40 fb40;
    BOOST_CHECK_EQUAL(fb40.capacity(),40);
    FixedByteArray64 fb64;
    BOOST_CHECK_EQUAL(fb64.capacity(),64);
    FixedByteArray128 fb128;
    BOOST_CHECK_EQUAL(fb128.capacity(),128);
    FixedByteArray256 fb256;
    BOOST_CHECK_EQUAL(fb256.capacity(),256);
    FixedByteArray512 fb512;
    BOOST_CHECK_EQUAL(fb512.capacity(),512);
    FixedByteArray1024 fb1024;
    BOOST_CHECK_EQUAL(fb1024.capacity(),1024);

    std::string str="Hello";

    /*
    //! Default ctor
    FixedByteArray() noexcept : FixedByteArrayBase(Capacity,ThrowOnOverflow)
    {}
    */
    FixedByteArray16 b1;
    BOOST_CHECK(b1.isEmpty());
    BOOST_CHECK_EQUAL(b1.size(),0);
    BOOST_CHECK_THROW(b1.at(0),std::out_of_range);
    BOOST_CHECK_THROW(b1.first(),std::out_of_range);
    BOOST_CHECK_THROW(b1.last(),std::out_of_range);

    /*
        //! Ctor from null-terminated string
        FixedByteArray(
            const char* data //!< String
        ):FixedByteArrayBase(Capacity,ThrowOnOverflow)
        {
            append(data);
        }
    */
    FixedByteArray16 b2(str.c_str());
    checkArray(str,b2);

    /*
        //! Ctor from data buffer
        FixedByteArray(
            const char* data, //!< Data buffer
            size_t size //!< Data size
        ):FixedByteArrayBase(Capacity,ThrowOnOverflow)
        {
            load(data,size);
        }
    */
    FixedByteArray16 b3(str.data(),str.size());
    checkArray(str,b3);

    /*
        //! Ctor from some container
        template <typename T> FixedByteArray(const T& other)
            : FixedByteArrayBase(Capacity,ThrowOnOverflow)
        {
            load(other.data(),other.size());
        }
    */
    FixedByteArray16 b4(str);
    checkArray(str,b4);

    /*
        //! Copy constuctor
        template <size_t capacity=Capacity, bool throwOverflow=ThrowOnOverflow>
        FixedByteArray(const FixedByteArray<capacity,throwOverflow>& other)
            : FixedByteArrayBase(Capacity,ThrowOnOverflow)
        {
            load(other.data(),other.size());
        }
    */
    // codechecker_false_positive [performance-unnecessary-copy-initialization]
    FixedByteArray16 b5(b4);
    checkArray(str,b5);

    /*
    //! Assignment operator
    template <typename T> FixedByteArray& operator=(const FixedByteArray& other)
    {
        if (&other!=this)
        {
            load(other.data(),other.size());
        }
        return *this;
    }
    */
    FixedByteArray16 b6;
    BOOST_CHECK(b6.isEmpty());
    b6=b5;
    checkArray(str,b6);

    /*
    //! Move constructor
    template <size_t capacity=Capacity, bool throwOverflow=ThrowOnOverflow>
    FixedByteArray(
            FixedByteArray<capacity,throwOverflow>&& other
        ):FixedByteArrayBase(Capacity,ThrowOnOverflow)
    {
        load(other.data(),other.size());
        other.m_size=0;
    }
    */
    FixedByteArray16 b7(std::move(b6));
    checkArray(str,b7);
    // codechecker_intentional [bugprone-use-after-move]
    BOOST_CHECK(b6.isEmpty());

    /*
    //! Move assignment operator
    template <typename T> FixedByteArray& operator=(T&& other)
    {
        if (this!=&other)
        {
            load(other.data(),other.size());
            other.m_size=0;
        }
        return *this;
    }
    */
    FixedByteArray16 b8;
    BOOST_CHECK(b8.isEmpty());
    b8=std::move(b7);
    checkArray(str,b8);
    // codechecker_intentional [bugprone-use-after-move]
    BOOST_CHECK(b7.isEmpty());

    /*
    //! Assignment operator
    template <typename T> FixedByteArray& operator=(const T& other)
    {
        load(other.data(),other.size());
        return *this;
    }
    */
    FixedByteArray16 b9;
    BOOST_CHECK(b9.isEmpty());
    b9=str;
    checkArray(str,b9);
}

BOOST_AUTO_TEST_CASE(FixedByteArrayOp)
{
    std::string str1="Hello";
    std::string str2=" world";
    std::string strCheck1=str1+str2;
    std::string strCheck2=str1+'a';
    std::string strCheck3=str1+'b';

    FixedByteArray16 b1(str1);
    b1.append(str2);
    checkArray(strCheck1,b1);

    FixedByteArray16 b2(str1);
    FixedByteArray16 b3(str2);
    b2.append(b3);
    checkArray(strCheck1,b2);

    FixedByteArray16 b4(str1);
    b4.append('a');
    checkArray(strCheck2,b4);

    FixedByteArray16 b5(str1);
    b5.push_back('a');
    checkArray(strCheck2,b5);

    b5[b5.size()-1]='b';
    checkArray(strCheck3,b5);
    BOOST_CHECK_EQUAL(b5.last(),'b');
    BOOST_CHECK_EQUAL(b5.first(),'H');

    FixedByteArray16 b6(str1);
    b6=b6+str2;
    checkArray(strCheck1,b6);

    FixedByteArray16 b7(str1);
    b7=b7+str2.c_str();
    checkArray(strCheck1,b7);

    FixedByteArray16 b8(str1);
    b8=b8+'a';
    checkArray(strCheck2,b8);

    std::string overflowStr="Let's overflow!!! Let's overflow!!! Let's overflow!!!";
    std::string overflowStrResult="Let's overflow!!";

    FixedByteArray16 b9(overflowStr);
    checkArray(overflowStrResult,b9);

    FixedByteArray16 b10;
    b10.append(overflowStr);
    checkArray(overflowStrResult,b10);
    BOOST_CHECK_THROW(b10.at(100),std::out_of_range);

    FixedByteArrayThrow16 b11;
    BOOST_CHECK_THROW(b11.append(overflowStr),std::overflow_error);

    BOOST_CHECK_THROW({FixedByteArrayThrow16 b12(overflowStr);},std::overflow_error);
}

template <typename T> void checkMakeFromString(const T& fb,size_t length)
{
    BOOST_CHECK_EQUAL(fb.capacity(),length);
}

BOOST_AUTO_TEST_CASE(FixedByteMakeFromString)
{
    checkMakeFromString(MakeFixedString("aaaaa"),8);
    checkMakeFromString(MakeFixedString("aaaaaaaaaa"),16);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaa"),20);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaa"),32);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),40);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),64);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),128);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),256);
    checkMakeFromString(MakeFixedString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          ),
         512);
}

BOOST_AUTO_TEST_CASE(FixedByteResize)
{
    std::string str1="Hello";
    std::string strCheck1="Hel";
    std::string strCheck2="HelLO WORLD";
    std::string strCheck3="HelLO WORLD, guy";

    FixedByteArray16 b1(str1);
    b1.resize(3);
    checkArray(strCheck1,b1);

    b1.resize(11);
    memcpy(b1.data()+3,strCheck2.data()+3,8);
    checkArray(strCheck2,b1);

    b1.resize(20);
    memcpy(b1.data()+11,strCheck3.data()+11,5);
    checkArray(strCheck3,b1);

    FixedByteArrayThrow16 b2(str1);
    BOOST_CHECK_THROW(b2.resize(20),std::overflow_error);
}

BOOST_AUTO_TEST_CASE(TestStrId)
{
    STR_ID_TYPE id1("some_id1");
    // codechecker_false_positive [performance-unnecessary-copy-initialization]
    STR_ID_SHARED_TYPE id2(id1);
    // codechecker_false_positive [performance-unnecessary-copy-initialization]
    STR_ID_SHARED_TYPE id3(id2);

    BOOST_CHECK_EQUAL(id1.c_str(),id2.c_str());
    BOOST_CHECK_EQUAL(id1.c_str(),id3.c_str());

    STR_ID_SHARED_TYPE id4;
    BOOST_CHECK_EQUAL(id4.c_str(),"unknown");
}

BOOST_AUTO_TEST_CASE(FixedByteArrayFill)
{
    size_t size=100;

    FixedByteArray256 b1;
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

BOOST_AUTO_TEST_CASE(FixedByteArrayCompare)
{
    FixedByteArray<16> arr1{"mthd"};
    auto less1=std::less<FixedByteArray<16>>{}(arr1,"req");
    BOOST_CHECK(less1);
    auto less2=std::less<FixedByteArray<16>>{}("req",arr1);
    BOOST_CHECK(!less2);
    auto less3=std::less<FixedByteArray<16>>{}(arr1,lib::string_view{"req"});
    BOOST_CHECK(less3);
    auto less4=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr1);
    BOOST_CHECK(!less4);

    FixedByteArray<16> arr2{"req"};
    auto less11=std::less<FixedByteArray<16>>{}(arr2,"req");
    BOOST_CHECK(!less11);
    auto less22=std::less<FixedByteArray<16>>{}("req",arr2);
    BOOST_CHECK(!less22);
    auto less33=std::less<FixedByteArray<16>>{}(arr2,lib::string_view{"req"});
    BOOST_CHECK(!less33);
    auto less44=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr2);
    BOOST_CHECK(!less44);

    FixedByteArray<16> arr3{"req1"};
    auto less111=std::less<FixedByteArray<16>>{}(arr3,"req");
    BOOST_CHECK(!less111);
    auto less222=std::less<FixedByteArray<16>>{}("req",arr3);
    BOOST_CHECK(less222);
    auto less333=std::less<FixedByteArray<16>>{}(arr3,lib::string_view{"req"});
    BOOST_CHECK(!less333);
    auto less444=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr3);
    BOOST_CHECK(less444);

    FixedByteArray<16> arr4{"re"};
    BOOST_CHECK(arr4.isLess("req"));
    auto less1111=std::less<FixedByteArray<16>>{}(arr4,"req");
    BOOST_CHECK(less1111);
    auto less2222=std::less<FixedByteArray<16>>{}("req",arr4);
    BOOST_CHECK(!less2222);
    auto less3333=std::less<FixedByteArray<16>>{}(arr4,lib::string_view{"req"});
    BOOST_CHECK(less3333);
    auto less4444=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr4);
    BOOST_CHECK(!less4444);

    FixedByteArray<16> arr5{"rep"};
    auto less11111=std::less<FixedByteArray<16>>{}(arr5,"req");
    BOOST_CHECK(less11111);
    auto less22222=std::less<FixedByteArray<16>>{}("req",arr5);
    BOOST_CHECK(!less22222);
    auto less33333=std::less<FixedByteArray<16>>{}(arr5,lib::string_view{"req"});
    BOOST_CHECK(less33333);
    auto less44444=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr5);
    BOOST_CHECK(!less44444);

    FixedByteArray<16> arr6{"res"};
    auto less111111=std::less<FixedByteArray<16>>{}(arr6,"req");
    BOOST_CHECK(!less111111);
    auto less222222=std::less<FixedByteArray<16>>{}("req",arr6);
    BOOST_CHECK(less222222);
    auto less333333=std::less<FixedByteArray<16>>{}(arr6,lib::string_view{"req"});
    BOOST_CHECK(!less333333);
    auto less444444=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr6);
    BOOST_CHECK(less444444);

    FixedByteArray<16> arr7{""};
    auto less1111111=std::less<FixedByteArray<16>>{}(arr7,"req");
    BOOST_CHECK(less1111111);
    auto less2222222=std::less<FixedByteArray<16>>{}("req",arr7);
    BOOST_CHECK(!less2222222);
    auto less3333333=std::less<FixedByteArray<16>>{}(arr7,lib::string_view{"req"});
    BOOST_CHECK(less3333333);
    auto less4444444=std::less<FixedByteArray<16>>{}(lib::string_view{"req"},arr7);
    BOOST_CHECK(!less4444444);

    FixedByteArray<16> arr8{"res"};
    auto less11111111=std::less<FixedByteArray<16>>{}(arr8,"");
    BOOST_CHECK(!less11111111);
    auto less22222222=std::less<FixedByteArray<16>>{}("",arr8);
    BOOST_CHECK(less22222222);
    auto less3333333333=std::less<FixedByteArray<16>>{}(arr8,lib::string_view{""});
    BOOST_CHECK(!less3333333333);
    auto less44444444=std::less<FixedByteArray<16>>{}(lib::string_view{""},arr8);
    BOOST_CHECK(less44444444);

    BOOST_CHECK(arr6==arr8);
    BOOST_CHECK(!(arr6<arr8));
    BOOST_CHECK(!(arr6>arr8));
    BOOST_CHECK(arr6<=arr8);
    BOOST_CHECK(arr6>=arr8);

    BOOST_CHECK(!(arr7==arr8));
    BOOST_CHECK((arr7<arr8));
    BOOST_CHECK(!(arr7>arr8));
    BOOST_CHECK(arr7<=arr8);
    BOOST_CHECK(!(arr7>=arr8));

    BOOST_CHECK(!(arr5==arr8));
    BOOST_CHECK((arr5<arr8));
    BOOST_CHECK(!(arr5>arr8));
    BOOST_CHECK(arr5<=arr8);
    BOOST_CHECK(!(arr5>=arr8));
}

BOOST_AUTO_TEST_SUITE_END()
