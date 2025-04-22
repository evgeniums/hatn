/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file section/sectionerror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/section/sectionerror.h>

HATN_SECTION_NAMESPACE_BEGIN

/********************** SectionErrorCategory **************************/

//---------------------------------------------------------------
const SectionErrorCategory& SectionErrorCategory::getCategory() noexcept
{
    static SectionErrorCategory SectionErrorCategoryInstance;
    return SectionErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string SectionErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_SECTION_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* SectionErrorCategory::codeString(int code) const
{
    return errorString(code,SectionErrorStrings);
}

//---------------------------------------------------------------

HATN_SECTION_NAMESPACE_END
