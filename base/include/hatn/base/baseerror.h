/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/baseerror.h
  *
  * Contains declarations of error helpers for hatnbase lib.
  *
  */

/****************************************************************************/

#ifndef HATNBASEERROR_H
#define HATNBASEERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerrorcodes.h>

HATN_BASE_NAMESPACE_BEGIN

//! Error category for hatnbase.
class HATN_BASE_EXPORT BaseErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.base";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const BaseErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code BaseError code.
 * @return Error object.
 */
inline Error baseError(BaseError code) noexcept
{
    return Error(static_cast<int>(code),&BaseErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code BaseError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error baseError(BaseError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&BaseErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

inline Error baseError(BaseError code, std::string msg) noexcept
{
    return baseError(code,std::make_shared<common::NativeError>(msg));
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASEERROR_H
