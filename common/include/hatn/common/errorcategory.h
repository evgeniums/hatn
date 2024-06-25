/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/errorcategory.h
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
#include <hatn/common/commonerrorcodes.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base error category.
class ErrorCategory : public std::error_category
{
    public:

        //! Get string representation of the code.
        virtual const char* codeString(int code) const=0;
};


//! Error category for hatncommonlib.
class HATN_COMMON_EXPORT CommonErrorCategory : public ErrorCategory
{
    public:

        //! Name of the category.
        virtual const char *name() const noexcept override
        {
            return "hatn.common";
        }

        //! Get description for the code.
        virtual std::string message(int code) const override;

        //! Get string representation of the code.
        virtual const char* codeString(int code) const;

        //! Get category.
        static const CommonErrorCategory& getCategory() noexcept;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNERRORCATEGORY_H
