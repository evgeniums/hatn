/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file journal/journalerrorcodes.h
  *
  * Contains error codes for hatnjournal lib.
  *
  */

/****************************************************************************/

#ifndef HATNJOURNALERRORCODES_H
#define HATNJOURNALERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/journal/journal.h>

#define HATN_JOURNAL_ERRORS(Do) \
    Do(JournalError,OK,_TR("OK")) \

HATN_JOURNAL_NAMESPACE_BEGIN

//! Error codes of hatnjournal lib.
enum class JournalError : int
{
    HATN_JOURNAL_ERRORS(HATN_ERROR_CODE)
};

//! journal errors codes as strings.
constexpr const char* const JournalErrorStrings[] = {
    HATN_JOURNAL_ERRORS(HATN_ERROR_STR)
};

//! Journal error code to string.
inline const char* journalErrorString(JournalError code)
{
    return errorString(code,JournalErrorStrings);
}

HATN_JOURNAL_NAMESPACE_END

#endif // HATNJOURNALERRORCODES_H
