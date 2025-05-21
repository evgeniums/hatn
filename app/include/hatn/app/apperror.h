/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/apperror.h
  *
  * Contains declarations of error helpers for hatnapp lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPPERROR_H
#define HATNAPPERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/app/appdefs.h>
#include <hatn/app/apperrorcodes.h>

HATN_APP_NAMESPACE_BEGIN

//! Error category for hatnapp.
class HATN_APP_EXPORT AppErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.app";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const AppErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code AppError code.
 * @return Error object.
 */
inline Error appError(AppError code) noexcept
{
    return Error(static_cast<int>(code),&AppErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code AppError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error appError(AppError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&AppErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_APP_NAMESPACE_END

#endif // HATNAPPERROR_H
