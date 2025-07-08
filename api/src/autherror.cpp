/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/autherror.сpp
  */

#include <hatn/common/translate.h>

#include <hatn/api/autherror.h>

HATN_API_NAMESPACE_BEGIN

/********************** ApiAuthErrorCategory **************************/

//---------------------------------------------------------------
const ApiAuthErrorCategory& ApiAuthErrorCategory::getCategory() noexcept
{
    static ApiAuthErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string ApiAuthErrorCategory::message(int code,const common::Translator* translator) const
{
    std::string result;
    switch (code)
    {
        HATN_API_AUTH_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error","auth",translator);
    }

    return result;
}

//---------------------------------------------------------------
const char* ApiAuthErrorCategory::status(int code) const
{
    return errorString(code,ApiAuthErrorStrings);
}

//---------------------------------------------------------------

HATN_API_NAMESPACE_END
