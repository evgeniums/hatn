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

#define HATN_DB_ERRORS(Do) \
    Do(DbError,OK,_TR("OK")) \
    Do(DbError,DUPLICATE_UNIQUE_KEY,_TR("duplicate unique key in database","db")) \
    Do(DbError,DB_ALREADY_OPEN,_TR("database connection already open","db")) \
    Do(DbError,DB_OPEN_FAILED,_TR("failed to open database connection","db")) \
    Do(DbError,DB_CLOSE_FAILED,_TR("failed to close database connection","db")) \
    Do(DbError,DB_CREATE_FAILED,_TR("failed to create database","db")) \
    Do(DbError,DB_DESTROY_FAILED,_TR("failed to destroy database","db")) \
    Do(DbError,DB_NOT_OPEN,_TR("database connection not open","db")) \
    Do(DbError,MODEL_NOT_FOUND,_TR("model not found","db")) \
    Do(DbError,SCHEMA_NOT_FOUND,_TR("schema not found","db")) \
    Do(DbError,SCHEMA_NOT_REGISTERED,_TR("schema not registered","db")) \
    Do(DbError,PARTITION_NOT_FOUND,_TR("partition not found","db")) \
    Do(DbError,COLLECTION_NOT_FOUND,_TR("collection not found","db")) \
    Do(DbError,DB_READ_ONLY,_TR("invalid request for read only database","db")) \
    Do(DbError,TX_BEGIN_FAILED,_TR("failed to validate configuration object","db")) \
    Do(DbError,TX_COMMIT_FAILED,_TR("failed to commit database transaction","db")) \
    Do(DbError,TX_ROLLBACK_FAILED,_TR("failed to rollback database transaction","db")) \
    Do(DbError,WRITE_OBJECT_FAILED,_TR("failed to write object to database","db")) \
    Do(DbError,READ_FAILED,_TR("failed to read object from database","db")) \
    Do(DbError,NOT_FOUND,_TR("not found","db")) \
    Do(DbError,EXPIRED,_TR("expired","db")) \
    Do(DbError,SAVE_INDEX_FAILED,_TR("failed to save index in database","db")) \
    Do(DbError,SAVE_TTL_INDEX_FAILED,_TR("failed to save ttl index in database","db")) \
    Do(DbError,PARTITION_CREATE_FALIED,_TR("failed to create database partition","db")) \
    Do(DbError,PARTITION_DELETE_FALIED,_TR("failed to delete database partition","db")) \
    Do(DbError,PARTITION_LIST_FAILED,_TR("failed to list database partitions","db")) \
    Do(DbError,DELETE_INDEX_FAILED,_TR("failed to delete index from database","db")) \
    Do(DbError,DELETE_OBJECT_FAILED,_TR("failed to delete object from database","db")) \
    Do(DbError,DELETE_TTL_INDEX_FAILED,_TR("failed to delete ttl index in database","db")) \
    Do(DbError,DELETE_TOPIC_FAILED,_TR("failed to delete topic from database","db")) \
    Do(DbError,TX_ROLLBACK,_TR("transaction rollback","db"))

HATN_DB_NAMESPACE_BEGIN

//! Error codes of hatndb lib.
enum class DbError : int
{
    HATN_DB_ERRORS(HATN_ERROR_CODE)
};

//! db errors codes as strings.
constexpr const char* const DbErrorStrings[] = {
    HATN_DB_ERRORS(HATN_ERROR_STR)
};

//! db error code to string.
inline const char* dbErrorString(DbError code)
{
    return errorString(code,DbErrorStrings);
}

HATN_DB_NAMESPACE_END

#endif // HATNDBERRORCODES_H
