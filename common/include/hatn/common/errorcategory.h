/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/errorcategory.h
  *
  *     Contains declarations of error categories.
  *
  */

/****************************************************************************/

#ifndef HATNERRORCATEGORY_H
#define HATNERRORCATEGORY_H

#include <string>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for error categories
class HATN_COMMON_EXPORT ErrorCategory
{
    public:

        //! Ctor
        ErrorCategory()=default;

        virtual ~ErrorCategory()=default;
        ErrorCategory(const ErrorCategory&)=delete;
        ErrorCategory(ErrorCategory&&) =delete;
        ErrorCategory& operator=(const ErrorCategory&)=delete;
        ErrorCategory& operator=(ErrorCategory&&) =delete;

        //! Name of the category
        virtual const char *name() const noexcept = 0;

        //! Get description for the code
        virtual std::string message(int code, const std::string& nativeMessage=std::string()) const = 0;

        inline bool operator==(const ErrorCategory &rhs) const noexcept { return this == &rhs; }
        inline bool operator!=(const ErrorCategory &rhs) const noexcept { return this != &rhs; }
        inline bool operator<( const ErrorCategory &rhs ) const noexcept
        {
            return std::less<const ErrorCategory*>()( this, &rhs );
        }
};

//! Common errors
enum class CommonError : int
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
    TIMEOUT=10
};

//! Generic error category
class HATN_COMMON_EXPORT CommonErrorCategory : public ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept override
        {
            return "hatn.common";
        }

        //! Get description for the code
        virtual std::string message(int code, const std::string& nativeMessage=std::string()) const override;

        //! Get category
        static const CommonErrorCategory& getCategory() noexcept;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNERRORCATEGORY_H
