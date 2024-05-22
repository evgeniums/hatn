/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/commonerrorcategory.h
  *
  *     Contains declarations of error category for hatncommon lib.
  *
  */

/****************************************************************************/

#ifndef HATNERRORCATEGORY_H
#define HATNERRORCATEGORY_H

#include <string>
#include <system_error>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Common errors
enum class ErrorCodes : int
{
    UNKNOWN=-1,
    OK=0,
    INVALID_SIZE=1,
    INVALID_ARGUMENT=2,
    UNSUPPORTED=3,
    INVALID_FILENAME=4,
    FILE_FLUSH_FAILED=5,
    FILE_ALREADY_OPEN=6,
    FILE_WRITE_FAILED=7,
    FILE_READ_FAILED=8,
    FILE_NOT_OPEN=9,
    TIMEOUT=10,
    RESULT_ERROR=11,
    RESULT_NOT_ERROR=12,
    NOT_IMPLEMENTED=13,
    NOT_FOUND=14
};

using CommonError=ErrorCodes;

//! Error category for hatncommonlib.
class HATN_COMMON_EXPORT CommonErrorCategory : public std::error_category
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.common";
        }

        //! Get description for the code
        virtual std::string message(int code) const override;

        //! Get category
        static const CommonErrorCategory& getCategory() noexcept;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNERRORCATEGORY_H
