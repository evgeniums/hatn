/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file section/sectionerror.h
  *
  * Contains declarations of error helpers for hatnsection lib.
  *
  */

/****************************************************************************/

#ifndef HATNSECTIONERROR_H
#define HATNSECTIONERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/section/section.h>
#include <hatn/section/sectionerrorcodes.h>

HATN_SECTION_NAMESPACE_BEGIN

//! Error category for hatnsection.
class HATN_SECTION_EXPORT SectionErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.section";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const SectionErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code SectionError code.
 * @return Error object.
 */
inline Error sectionError(SectionError code) noexcept
{
    return Error(static_cast<int>(code),&SectionErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code SectionError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error sectionError(SectionError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&SectionErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_SECTION_NAMESPACE_END

#endif // HATNSECTIONERROR_H
