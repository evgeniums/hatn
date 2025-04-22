/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file section/sectionerrorcodes.h
  *
  * Contains error codes for hatnsection lib.
  *
  */

/****************************************************************************/

#ifndef HATNSECTIONERRORCODES_H
#define HATNSECTIONERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/section/section.h>

#define HATN_SECTION_ERRORS(Do) \
    Do(SectionError,OK,_TR("OK")) \
    Do(SectionError,UNKNOWN_TOPIC,_TR("unknown topic")) \

HATN_SECTION_NAMESPACE_BEGIN

//! Error codes of hatnsection lib.
enum class SectionError : int
{
    HATN_SECTION_ERRORS(HATN_ERROR_CODE)
};

//! section errors codes as strings.
constexpr const char* const SectionErrorStrings[] = {
    HATN_SECTION_ERRORS(HATN_ERROR_STR)
};

//! Section error code to string.
inline const char* sectionErrorString(SectionError code)
{
    return errorString(code,SectionErrorStrings);
}

HATN_SECTION_NAMESPACE_END

#endif // HATNSECTIONERRORCODES_H
