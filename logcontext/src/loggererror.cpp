/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/logcontexterror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/logcontext/loggererror.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

/********************** LogContextErrorCategory **************************/

//---------------------------------------------------------------
const LogContextErrorCategory& LogContextErrorCategory::getCategory() noexcept
{
    static LogContextErrorCategory LogContextErrorCategoryInstance;
    return LogContextErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string LogContextErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_LOGCONTEXT_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* LogContextErrorCategory::codeString(int code) const
{
    return errorString(code,LogContextErrorStrings);
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
