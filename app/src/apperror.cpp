/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/apperror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/app/apperror.h>

HATN_APP_NAMESPACE_BEGIN

/********************** AppErrorCategory **************************/

//---------------------------------------------------------------
const AppErrorCategory& AppErrorCategory::getCategory() noexcept
{
    static AppErrorCategory AppErrorCategoryInstance;
    return AppErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string AppErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_APP_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* AppErrorCategory::codeString(int code) const
{
    return errorString(code,AppErrorStrings);
}

//---------------------------------------------------------------

HATN_APP_NAMESPACE_END
