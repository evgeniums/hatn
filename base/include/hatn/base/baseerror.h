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
#include <hatn/common/result.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerrorcodes.h>

HATN_BASE_NAMESPACE_BEGIN

//! Error category for hatnbase.
class HATN_BASE_EXPORT BaseErrorCategory : public std::error_category
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.base";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get category
    static const BaseErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code..
 * @param code BaseError code.
 * @return Error object.
 */
inline Error baseError(BaseError code) noexcept
{
    return Error(static_cast<int>(code),&BaseErrorCategory::getCategory());
}

/**
 * @brief Make ErrorResult from baseError
 * @param code BaseError code
 * @return ErrorResult
 */
inline ErrorResult baseErrorResult(BaseError code) noexcept
{
    return ErrorResult{baseError(code)};
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASEERROR_H
