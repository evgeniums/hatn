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

//! API error is used to hold information to be sent back as a result of some request or API command.
class HATN_COMMON_EXPORT ApiError
{
    public:

        virtual ~ApiError();

        ApiError(bool enable=false) : m_selfApiError(enable)
        {}

        ApiError(const ApiError&)=default;
        ApiError(ApiError&&)=default;
        ApiError& operator=(const ApiError&)=default;
        ApiError& operator=(ApiError&&)=default;

        void enableApiError(bool enable)
        {
            m_selfApiError=enable;
        }

        bool isApiErrorEnabled() const noexcept
        {
            return m_selfApiError;
        }

        virtual const ApiError* apiError() const noexcept
        {
            if (m_selfApiError)
            {
                return this;
            }
            return nullptr;
        }

        virtual const void* apiData() const noexcept
        {
            return nullptr;
        }

        virtual int apiCode() const noexcept
        {
            return 0;
        }

        virtual std::string apiMessage() const
        {
            return std::string();
        }

        virtual std::string apiFamily() const
        {
            return std::string();
        }

        virtual ConstDataBuf apiWireData() const
        {
            return ConstDataBuf{};
        }

    private:

        bool m_selfApiError;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
