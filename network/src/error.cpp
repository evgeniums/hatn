/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/error.сpp
  *
  *     Error classes for Hatn Network Library
  *
  */

#include <hatn/common/translate.h>
#include <hatn/network/networkerror.h>

HATN_NETWORK_NAMESPACE_BEGIN

/********************** NetworkErrorCategory **************************/

//---------------------------------------------------------------

const NetworkErrorCategory& NetworkErrorCategory::getCategory() noexcept
{
    static NetworkErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------

std::string NetworkErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_NETWORK_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------

const char* NetworkErrorCategory::codeString(int code) const
{
    return errorString(code,NetworkErrorStrings);
}

//---------------------------------------------------------------
    
HATN_NETWORK_NAMESPACE_END
