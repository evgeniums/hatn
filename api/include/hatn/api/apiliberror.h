/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/apiliberror.h
  *
  * Contains declarations of error helpers for hatnapi lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPILIBERROR_H
#define HATNAPILIBERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberrorcodes.h>

HATN_API_NAMESPACE_BEGIN

//! Error category for hatnapi.
class HATN_API_EXPORT ApiLibErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.api";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const ApiLibErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code ApiError code.
 * @return Error object.
 */
inline Error apiLibError(ApiLibError code) noexcept
{
    return Error(static_cast<int>(code),&ApiLibErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code ApiError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error apiLibError(ApiLibError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&ApiLibErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_API_NAMESPACE_END

#endif // HATNAPIERROR_H
