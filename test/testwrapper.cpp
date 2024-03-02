#include "hatn_test_config.h"
#ifdef HATN_TEST_WRAP_C

#include <string>
#include <iostream>

#define BOOST_TEST_MODULE DracoshaLibsTest
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

extern "C" {

#ifdef _WIN32
    #define HATN_TEST_EXPORT __declspec(dllexport)
#else
    #define HATN_TEST_EXPORT
#endif

HATN_TEST_EXPORT int testConsole(const char* assetsPath, const char* tmpPath)
{
    hatn::test::MultiThreadFixture::setAssetsPath(std::string(assetsPath));
    hatn::test::MultiThreadFixture::setTmpPath(std::string(tmpPath));

    const char* argv[]={"hatnlibs-test","--log_level=test_suite"};
    int argc = 2;
    return boost::unit_test::unit_test_main(::init_unit_test, argc, const_cast<char**>(argv));
}

HATN_TEST_EXPORT int testJUnit(const char* assetsPath, const char* tmpPath)
{
    hatn::test::MultiThreadFixture::setAssetsPath(std::string(assetsPath));
    hatn::test::MultiThreadFixture::setTmpPath(std::string(tmpPath));

    std::string path=hatn::test::MultiThreadFixture::tmpPath()+"/junit.xml";
    path=std::string("--log_sink=")+path;
    std::cout<<"Path="<<path<<std::endl;
    int argc=6;
    const char* argv[]={
                "hatnlibs-test",
                "--log_format=JUNIT",
                  path.c_str(),
                  "--log_level=all",
                  "--report_level=no",
                  "--result_code=no"};
    return boost::unit_test::unit_test_main(::init_unit_test, argc, const_cast<char**>(argv));
}

}
#endif
