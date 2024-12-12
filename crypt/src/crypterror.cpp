/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/crypterror.cpp
  *
  *   Definitions for errors of crypt module
  *
  */

/****************************************************************************/

#include <hatn/common/translate.h>

#include <hatn/crypt/crypterror.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** CryptErrorCategory **************************/

//---------------------------------------------------------------
CryptErrorCategory& CryptErrorCategory::getCategory() noexcept
{
    static CryptErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string CryptErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_CRYPT_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }
    return result;
}

//---------------------------------------------------------------
const char* CryptErrorCategory::codeString(int code) const
{
    return errorString(code,CryptErrorStrings);
}

//---------------------------------------------------------------

HATN_CRYPT_NAMESPACE_END
