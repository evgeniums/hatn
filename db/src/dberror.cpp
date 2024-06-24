/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/error.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/db/dberror.h>

HATN_DB_NAMESPACE_BEGIN

/********************** DbErrorCategory **************************/

//---------------------------------------------------------------
const DbErrorCategory& DbErrorCategory::getCategory() noexcept
{
    static DbErrorCategory DbErrorCategoryInstance;
    return DbErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string DbErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_DB_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

HATN_DB_NAMESPACE_END
