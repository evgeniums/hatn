/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientapperror.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPERROR_H
#define HATNCLIENTAPPERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/clientapperrorcodes.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//! Error category for hatnapi.
class HATN_CLIENTAPP_EXPORT ClientAppErrorCategory : public common::ErrorCategory
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
    static const ClientAppErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code ApiError code.
 * @return Error object.
 */
inline Error clientAppError(ClientAppError code) noexcept
{
    return Error(static_cast<int>(code),&ClientAppErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code ApiError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error clientAppError(ClientAppError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&ClientAppErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPERROR_H
