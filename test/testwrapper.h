#ifndef HATN_TEST_WRAPPER_H
#define HATN_TEST_WRAPPER_H

#ifdef _WIN32
    #define HATN_TEST_IMPORT __declspec(dllimport)
#else
    #define HATN_TEST_IMPORT
#endif

HATN_TEST_IMPORT int testConsole(const char* assetsPath, const char* tmpPath);
HATN_TEST_IMPORT int testJUnit(const char* assetsPath, const char* tmpPath);

#endif
