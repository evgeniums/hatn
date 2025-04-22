/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file journal/journalerror.h
  *
  * Contains declarations of error helpers for hatnjournal lib.
  *
  */

/****************************************************************************/

#ifndef HATNJOURNALERROR_H
#define HATNJOURNALERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/journal/journal.h>
#include <hatn/journal/journalerrorcodes.h>

HATN_JOURNAL_NAMESPACE_BEGIN

//! Error category for hatnjournal.
class HATN_JOURNAL_EXPORT JournalErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.journal";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const JournalErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code JournalError code.
 * @return Error object.
 */
inline Error journalError(JournalError code) noexcept
{
    return Error(static_cast<int>(code),&JournalErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code JournalError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error journalError(JournalError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&JournalErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_JOURNAL_NAMESPACE_END

#endif // HATNJOURNALERROR_H
