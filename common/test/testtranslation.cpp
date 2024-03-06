#include <iostream>
#include <fstream>

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

using namespace std;

BOOST_AUTO_TEST_SUITE(TranslatorTest)

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
    auto help=_TR("help");
    BOOST_CHECK_EQUAL(help, fromFile("help." + encoding + ".txt"));

    /* From different dictionaries */
    auto string1=_TR("string1");
    BOOST_CHECK_EQUAL(string1, fromFile("string1." + encoding + ".txt"));
    auto string2=_TR("string2");
    BOOST_CHECK_EQUAL(string2, fromFile("string2." + encoding + ".txt"));

    /* Not existing */
    BOOST_CHECK_EQUAL(_TR("something"), "something");
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

#ifdef WIN32

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


#ifdef TEST_NON_UNICODE_WINDOWS
//! @todo Some other system parameters need to be checked also for this test
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
