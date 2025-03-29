/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/baseerror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

/********************** BaseErrorCategory **************************/

//---------------------------------------------------------------
const BaseErrorCategory& BaseErrorCategory::getCategory() noexcept
{
    static BaseErrorCategory BaseErrorCategoryInstance;
    return BaseErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string BaseErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_BASE_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* BaseErrorCategory::codeString(int code) const
{
    return errorString(code,BaseErrorStrings);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
