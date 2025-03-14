/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/apiliberror.сpp
  *
  *      Contains definition of error category for hatn API library.
  *
  */

#include <hatn/common/translate.h>

#include <hatn/api/apiliberror.h>

HATN_API_NAMESPACE_BEGIN

/********************** BaseErrorCategory **************************/

//---------------------------------------------------------------
const ApiLibErrorCategory& ApiLibErrorCategory::getCategory() noexcept
{
    static ApiLibErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string ApiLibErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_API_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* ApiLibErrorCategory::codeString(int code) const
{
    return errorString(code,ApiErrorStrings);
}

//---------------------------------------------------------------

HATN_API_NAMESPACE_END
