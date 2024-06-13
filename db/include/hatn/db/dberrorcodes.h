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
    DB_ALREADY_OPEN,
    DB_OPEN_FAILED,
    DB_CLOSE_FAILED,
    DB_CREATE_FAILED,
    DB_DESTROY_FAILED,
    DB_NOT_OPEN,
    MODEL_NOT_FOUND,
    SCHEMA_NOT_FOUND,
    PARTITION_NOT_FOUND,
    COLLECTION_NOT_FOUND,
    DB_READ_ONLY,
    TX_BEGIN_FAILED,
    TX_COMMIT_FAILED,
    WRITE_OBJECT_FAILED,
    PARTITION_CREATE_FALIED,
    PARTITION_DELETE_FALIED,
    PARTITION_LIST_FAILED
};

HATN_DB_NAMESPACE_END

#endif // HATNDBERRORCODES_H
