/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/logcontexterror.h
  *
  * Contains declarations of error helpers for hatnlogcontext lib.
  *
  */

/****************************************************************************/

#ifndef HATNLOGCONTEXTERROR_H
#define HATNLOGCONTEXTERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/loggererrorcodes.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

//! Error category for hatnlogcontext.
class HATN_LOGCONTEXT_EXPORT LogContextErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.logcontext";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const LogContextErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code LogContextError code.
 * @return Error object.
 */
inline Error logcontextError(LogContextError code) noexcept
{
    return Error(static_cast<int>(code),&LogContextErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code LogContextError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error logcontextError(LogContextError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&LogContextErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNLOGCONTEXTERROR_H
