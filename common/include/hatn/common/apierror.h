/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/apierror.h
  *
  *     Contains definition of API error class.
  *
  */

/****************************************************************************/

#ifndef HATNAPIERROR_H
#define HATNAPIERROR_H

#include <hatn/common/format.h>

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/spanbuffer.h>
#include <hatn/common/result.h>

HATN_COMMON_NAMESPACE_BEGIN

//! API error is used to hold information to be sent back or shown as a result of some request or API command.
class HATN_COMMON_EXPORT ApiError : public NativeError
{
    public:

        using NativeError::NativeError;

        virtual const ApiError* apiError() const noexcept override
        {
            return this;
        }

        virtual const void* apiData() const noexcept
        {
            return nullptr;
        }

        virtual int apiCode() const noexcept
        {
            return nativeCode();
        }

        virtual std::string apiMessage() const
        {
            return nativeMessage();
        }

        virtual std::string apiFamily() const
        {
            if (category()!=nullptr)
            {
                return category()->name();
            }
            return std::string();
        }

        virtual Result<SpanBuffer> apiWireData() const
        {
            return Error{CommonError::UNSUPPORTED};
        }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
