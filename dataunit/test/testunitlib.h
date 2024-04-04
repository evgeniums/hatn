#pragma once

#ifdef WIN32
#    ifndef TEST_UNIT_EXPORT
#        ifdef BUILD_TEST_UNIT
#            define TEST_UNIT_EXPORT __declspec(dllexport)
#        else
#            define TEST_UNIT_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define TEST_UNIT_EXPORT
#endif
