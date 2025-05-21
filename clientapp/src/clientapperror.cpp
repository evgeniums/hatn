/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientapperror.сpp
  */

#include <hatn/common/translate.h>

#include <hatn/clientapp/clientapperror.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

/********************** ClientAppErrorCategory **************************/

//---------------------------------------------------------------
const ClientAppErrorCategory& ClientAppErrorCategory::getCategory() noexcept
{
    static ClientAppErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string ClientAppErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_CLIENTAPP_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* ClientAppErrorCategory::codeString(int code) const
{
    return errorString(code,ClientAppErrorStrings);
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
