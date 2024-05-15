/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/dberrorcodes.h
  *
  * Contains error codes for hatndb lib.
  *
  */

/****************************************************************************/

#ifndef HATNDBERRORCODES_H
#define HATNDBERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

//! Error codes of hatndb lib.
enum class DbError : int
{
    OK=static_cast<int>(common::CommonError::OK),
    ALREADY_OPENED,
    OPEN_FAILED,
    CLOSE_FAILED
};

HATN_DB_NAMESPACE_END

#endif // HATNDBERRORCODES_H
