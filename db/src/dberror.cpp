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
        case (static_cast<int>(DbError::OK)):
            result=common::CommonErrorCategory::getCategory().message(code);
            break;

        case (static_cast<int>(DbError::DB_ALREADY_OPENED)):
            result=_TR("database connection already opened","db");
            break;

        case (static_cast<int>(DbError::DB_OPEN_FAILED)):
            result=_TR("failed to open database connection","db");
            break;

        case (static_cast<int>(DbError::DB_CLOSE_FAILED)):
            result=_TR("failed to close database connection","db");
            break;

        case (static_cast<int>(DbError::DB_CREATE_FAILED)):
            result=_TR("failed to create database","db");
            break;

        case (static_cast<int>(DbError::DB_DESTROY_FAILED)):
            result=_TR("failed to destroy database","db");
            break;

        default:
            result=_TR("unknown error");
    }

    return result;
}

HATN_DB_NAMESPACE_END
