/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/mqerrorcodes.h
  *
  * Defines error codes for hatnmq lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPIERRORCODES_H
#define HATNAPIERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/mq/mq.h>

#define HATN_MQ_ERRORS(Do) \
    Do(MqError,OK,_TR("OK")) \
    Do(MqError,DUPLICATE_OBJECT_ID,_TR("Object with such ID was already posted to message queue")) \
    Do(MqError,INVALID_UPDATE_MESSAGE,_TR("Invalid update object")) \

HATN_MQ_NAMESPACE_BEGIN

//! Error codes of hatnmq lib.
enum class MqError : int
{
    HATN_MQ_ERRORS(HATN_ERROR_CODE)
};

//! base errors codes as strings.
constexpr const char* const MqErrorStrings[] = {
    HATN_MQ_ERRORS(HATN_ERROR_STR)
};

//! Base error code to string.
inline const char* mqErrorString(MqError code)
{
    return errorString(code,MqErrorStrings);
}

HATN_MQ_NAMESPACE_END

#endif // HATNAPIERRORCODES_H
