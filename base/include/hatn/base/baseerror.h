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

#include <hatn/base/base.h>
#include <hatn/base/baseerrorcodes.h>

HATN_BASE_NAMESPACE_BEGIN

//! Error category for hatnbase.
class HATN_BASE_EXPORT BaseErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept
    {
        return "hatn.base";
    }

    //! Get description for the code
    virtual std::string message(int code, const std::string& nativeMessage=std::string()) const;

    //! Get category
    static const BaseErrorCategory& getCategory() noexcept;
};

//! Make network error object from code
inline common::Error makeError(ErrorCode code) noexcept
{
    return common::Error(static_cast<int>(code),&BaseErrorCategory::getCategory());
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASEERROR_H
