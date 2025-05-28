/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientserver/clientservererror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/clientserver/clientservererror.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

/********************** ClientserverErrorCategory **************************/

//---------------------------------------------------------------
const ClientServerErrorCategory& ClientServerErrorCategory::getCategory() noexcept
{
    static ClientServerErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string ClientServerErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_CLIENTSERVER_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* ClientServerErrorCategory::codeString(int code) const
{
    return errorString(code,ClientServerErrorStrings);
}

//---------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END
