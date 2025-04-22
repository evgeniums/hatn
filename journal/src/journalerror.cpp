/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file journal/journalerror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/journal/journalerror.h>

HATN_JOURNAL_NAMESPACE_BEGIN

/********************** JournalErrorCategory **************************/

//---------------------------------------------------------------
const JournalErrorCategory& JournalErrorCategory::getCategory() noexcept
{
    static JournalErrorCategory JournalErrorCategoryInstance;
    return JournalErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string JournalErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_JOURNAL_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* JournalErrorCategory::codeString(int code) const
{
    return errorString(code,JournalErrorStrings);
}

//---------------------------------------------------------------

HATN_JOURNAL_NAMESPACE_END
