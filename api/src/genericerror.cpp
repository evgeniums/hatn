/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/genericerror.сpp
  */

#include <hatn/common/translate.h>

#include <hatn/api/genericerror.h>

HATN_API_NAMESPACE_BEGIN

/********************** BaseErrorCategory **************************/

//---------------------------------------------------------------
const ApiGenericErrorCategory& ApiGenericErrorCategory::getCategory() noexcept
{
    static ApiGenericErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string ApiGenericErrorCategory::message(int code,const common::Translator* translator) const
{
    std::string result;
    switch (code)
    {
        HATN_API_RESPONSE_STATUS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error","generic",translator);
    }

    return result;
}

//---------------------------------------------------------------
const char* ApiGenericErrorCategory::status(int code) const
{
    return errorString(code,protocol::ResponseStatusStrings);
}

//---------------------------------------------------------------

HATN_API_NAMESPACE_END
