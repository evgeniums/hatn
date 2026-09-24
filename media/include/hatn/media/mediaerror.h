/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/mediaerror.h
  *
  * Contains declarations of error helpers for hatnmedia lib.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAERROR_H
#define HATNMEDIAERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

#include <hatn/media/media.h>
#include <hatn/media/mediaerrorcodes.h>

HATN_MEDIA_NAMESPACE_BEGIN

//! Error category for hatnmedia.
class HATN_MEDIA_EXPORT MediaErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.media";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const MediaErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code MediaError code.
 * @return Error object.
 */
inline Error mediaError(MediaError code) noexcept
{
    return Error(static_cast<int>(code),&MediaErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and the lower-level error that caused it.
 * @param code MediaError code.
 * @param ec Underlying error, typically a file error, kept in the chain. If it is null (for
 *        example a short write that reported no error) nothing is chained.
 * @return Error object.
 */
inline Error mediaError(MediaError code, Error ec) noexcept
{
    if (ec.isNull())
    {
        return mediaError(code);
    }
    return common::chainErrors(std::move(ec),Error(code,&MediaErrorCategory::getCategory()));
}

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAERROR_H
