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

#include <string>

#include <hatn/common/common.h>
#include <hatn/common/databuf.h>

HATN_COMMON_NAMESPACE_BEGIN

class Error;

//! API error is used to hold information to be sent back as a result of some request or API command.
class HATN_COMMON_EXPORT ApiError
{
    public:

        constexpr static const char* DefaultStatus="success";

        virtual ~ApiError();

        ApiError(const Error* error=nullptr):m_error(error)
        {}

        ApiError(const ApiError&)=default;
        ApiError(ApiError&&)=default;
        ApiError& operator=(const ApiError&)=default;
        ApiError& operator=(ApiError&&)=default;

        void setError(const Error* error)
        {
            m_error=error;
        }

        virtual int apiCode() const noexcept;
        virtual const char* apiStatus() const noexcept;
        virtual std::string apiMessage() const;
        virtual std::string apiFamily() const;

        virtual const void* apiData() const noexcept
        {
            return nullptr;
        }

        virtual ConstDataBuf apiWireData() const
        {
            return ConstDataBuf{};
        }

    private:

        const Error* m_error;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
