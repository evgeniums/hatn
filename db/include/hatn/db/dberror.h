/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/dberror.h
  *
  * Contains declarations of error helpers for hatndb lib.
  *
  */

/****************************************************************************/

#ifndef HATNDBERROR_H
#define HATNDBERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/db/db.h>
#include <hatn/db/dberrorcodes.h>

HATN_DB_NAMESPACE_BEGIN

//! Error category for hatndb.
class HATN_DB_EXPORT DbErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.db";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const DbErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code DbError code.
 * @return Error object.
 */
inline Error dbError(DbError code) noexcept
{
    return Error(static_cast<int>(code),&DbErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code DbError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error dbError(DbError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&DbErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

/**
 * @brief Make ErrorResult from dbError
 * @param code DbError code
 * @return ErrorResult
 */
inline ErrorResult dbErrorResult(DbError code) noexcept
{
    return ErrorResult{dbError(code)};
}

inline void setDbErrorCode(Error& ec, DbError code)
{
    ec.setCode(code,&DbErrorCategory::getCategory());
}

HATN_DB_NAMESPACE_END

#endif // HATNBASEERROR_H
