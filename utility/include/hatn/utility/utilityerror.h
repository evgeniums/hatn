/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file utility/utilityerror.h
  *
  * Contains declarations of error helpers for hatnutility lib.
  *
  */

/****************************************************************************/

#ifndef HATNUTILITYERROR_H
#define HATNUTILITYERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/utilityerrorcodes.h>

HATN_UTILITY_NAMESPACE_BEGIN

//! Error category for hatnutility.
class HATN_UTILITY_EXPORT UtilityErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.utility";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const UtilityErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code UtilityError code.
 * @return Error object.
 */
inline Error utilityError(UtilityError code) noexcept
{
    return Error(static_cast<int>(code),&UtilityErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code UtilityError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error utilityError(UtilityError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&UtilityErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYERROR_H
