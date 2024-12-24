#include <iostream>
#include <fstream>

#include <boost/locale.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <hatn/common/application.h>
#include <hatn/common/translate.h>
#include <hatn/common/logger.h>

#include <hatn/test/multithreadfixture.h>

#ifdef WIN32
#include <windows.h>
#endif

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestTranslator)

inline static std::string fromFile(const std::string &fname)
{
    std::string fileName=fmt::format("{}/common/assets/{}",test::MultiThreadFixture::assetsPath(),fname);

    std::ifstream ss(fileName);
    std::string s;
    getline(ss, s);
    return s;
}

static void doTest(const std::string &encoding)
{
    /* From common */
    auto help=hatn::_TR("help");
    BOOST_CHECK_EQUAL(help, fromFile("help." + encoding + ".txt"));

    /* From different dictionaries */
    auto string1=hatn::_TR("string1");
    BOOST_CHECK_EQUAL(string1, fromFile("string1." + encoding + ".txt"));
    auto string2=hatn::_TR("string2");
    BOOST_CHECK_EQUAL(string2, fromFile("string2." + encoding + ".txt"));

    /* Not existing */
    BOOST_CHECK_EQUAL(hatn::_TR("something"), "something");
}

BOOST_AUTO_TEST_CASE(Utf8Encoding)
{
    BoostTranslatorFactory tr;
    tr.addMessagesDomain("hatncommon");
    tr.addMessagesDomain("test1");
    tr.addMessagesDomain("test2");
    tr.addDictionaryPath("translations");

    Translator::setTranslator(tr.create("ru_RU.UTF-8"));
    doTest("utf8");
}

BOOST_AUTO_TEST_CASE(Utf8Operations)
{
    std::string one{"Раз"};
    boost::locale::generator gen;
    auto ruLoc=gen.generate("ru_RU.UTF-8");
    auto lowerOne=boost::locale::to_lower(one,ruLoc);
    std::string sampleLowerOne{"раз"};
    BOOST_CHECK_EQUAL(lowerOne,sampleLowerOne);

    BOOST_TEST_MESSAGE("Original vector of strings:");
    std::vector<std::string> strings{"бард","Волга","Барс","Ангара","Алмаз","алмаз"};
    size_t i=0;
    for (auto&& it:strings)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;

    auto strings1=strings;
    std::sort(strings1.begin(),strings1.end());
    BOOST_TEST_MESSAGE("Sorted strings using default sorting comparator:");
    i=0;
    for (auto&& it:strings1)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;
    std::vector<std::string> checkStrings1{"Алмаз","Ангара","Барс","Волга","алмаз","бард"};
    BOOST_CHECK(strings1==checkStrings1);

    auto utfLoc=gen.generate("en_US.UTF-8");
    auto strings2=strings;
    std::sort(strings2.begin(),strings2.end(),[&utfLoc](const std::string& l, const std::string& r)
              {
                auto ll=boost::locale::to_lower(l,utfLoc);
                auto lr=boost::locale::to_lower(r,utfLoc);
                if (ll==lr)
                {
                    return l<r;
                }
                return ll<lr;
              }
        );
    BOOST_TEST_MESSAGE("Case insensitive sorted strings in en_US.UTF-8 locale using trivial less:");
    i=0;
    for (auto&& it:strings2)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;
    std::vector<std::string> checkStrings2{"Алмаз","алмаз","Ангара","бард","Барс","Волга"};
    BOOST_CHECK(strings2==checkStrings2);

    auto strings3=strings;
    std::sort(strings3.begin(),strings3.end(),[&utfLoc](const std::string& l, const std::string& r)
              {
                  auto ll=boost::locale::to_lower(l,utfLoc);
                  auto lr=boost::locale::to_lower(r,utfLoc);
                  if (ll==lr)
                  {
                      return l<r;
                  }
                  return ll<lr;
              }
              );
    BOOST_TEST_MESSAGE("Case insensitive sorted strings in en_US.UTF-8 locale using trivial less:");
    i=0;
    for (auto&& it:strings3)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;
    std::vector<std::string> checkStrings3{"Алмаз","алмаз","Ангара","бард","Барс","Волга"};
    BOOST_CHECK(strings3==checkStrings3);

    auto strings4=strings;
    boost::locale::comparator<char> comp1{utfLoc,boost::locale::collate_level::primary};
    BOOST_CHECK(!comp1(one,sampleLowerOne));
    BOOST_CHECK(!comp1(sampleLowerOne,one));
    std::sort(strings4.begin(),strings4.end(),comp1);
    BOOST_TEST_MESSAGE("Case insensitive sorted strings in en_US.UTF-8 locale using boost::locale::comparator");
    i=0;
    for (auto&& it:strings4)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;
    std::vector<std::string> checkStrings4{"Алмаз","алмаз","Ангара","бард","Барс","Волга"};
    BOOST_CHECK(strings4==checkStrings4);

    auto strings5=strings;
    boost::locale::comparator<char> comp2{utfLoc,boost::locale::collate_level::tertiary};
    BOOST_CHECK(!comp2(one,lowerOne));
    BOOST_CHECK(comp2(lowerOne,one));
    std::sort(strings5.begin(),strings5.end(),comp2);
    BOOST_TEST_MESSAGE("Case sensitive sorted strings in en_US.UTF-8 locale using boost::locale::comparator");
    i=0;
    for (auto&& it:strings5)
    {
        if (i!=0)
        {
            std::cout<<",";
        }
        std::cout<<it;
        i++;
    }
    std::cout<<std::endl;
    std::vector<std::string> checkStrings5{"алмаз","Алмаз","Ангара","бард","Барс","Волга"};
    BOOST_CHECK(strings5==checkStrings5);

    std::string latin{"Hello 12345678 0xABCDEF!"};
    std::string sampleLatinLower{"hello 12345678 0xabcdef!"};
    auto latinLower=boost::locale::to_lower(latin,ruLoc);
    BOOST_CHECK_EQUAL(sampleLatinLower,latinLower);
}

#ifdef WIN32

#ifdef TEST_NON_UNICODE_WINDOWS

BOOST_AUTO_TEST_CASE(WindowsEncoding)
{
    BoostTranslatorFactory tr;
    tr.addMessagesDomain("hatncommon");
    tr.addMessagesDomain("test1");
    tr.addMessagesDomain("test2");
    tr.addDictionaryPath("translations");

    auto translator = tr.create(false,BoostTranslatorFactory::WinConvertOEM::Disable);
    Translator::setTranslator(translator);
    doTest("1251");
}


BOOST_AUTO_TEST_CASE(WindowsConsole)
{
    if (GetOEMCP()==866)
    {
        BOOST_TEST_MESSAGE("OEM code page is 866, checking translator for that CP");

        BoostTranslatorFactory tr;
        tr.addMessagesDomain("hatncommon");
        tr.addMessagesDomain("test1");
        tr.addMessagesDomain("test2");
        tr.addDictionaryPath("translations");

        auto translator = tr.create(false,BoostTranslatorFactory::WinConvertOEM::Enable);
        Translator::setTranslator(translator);
        doTest("866");

        if (GetConsoleCP()==866)
        {
            BOOST_TEST_MESSAGE("Console code page is 866, checking translator with auto OEM conversion");

            translator = tr.create(false,BoostTranslatorFactory::WinConvertOEM::Auto);
            Translator::setTranslator(translator);
            doTest("866");
        }
    }
    else
    {
        BOOST_CHECK(true);
    }
}

#endif

#endif // WIN32

BOOST_AUTO_TEST_SUITE_END()
