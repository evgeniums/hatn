/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file utility/aclerror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/utility/utilityerror.h>

HATN_UTILITY_NAMESPACE_BEGIN

/********************** UtilityErrorCategory **************************/

//---------------------------------------------------------------
const UtilityErrorCategory& UtilityErrorCategory::getCategory() noexcept
{
    static UtilityErrorCategory UtilityErrorCategoryInstance;
    return UtilityErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string UtilityErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_UTILITY_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* UtilityErrorCategory::codeString(int code) const
{
    return errorString(code,UtilityErrorStrings);
}

//---------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END
