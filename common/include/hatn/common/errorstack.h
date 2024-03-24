/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/errorstack.h
  *
  *     Contains definition of native error representing a stack of errors.
  *
  */

/****************************************************************************/

#ifndef HATNERRORSTACK_H
#define HATNERRORSTACK_H

#include <hatn/common/format.h>

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Stack of errors.
class HATN_COMMON_EXPORT ErrorStack : public NativeError
{
    public:

        //! Ctor
        ErrorStack(
                Error prevError,
                std::string nativeMessage,
                int nativeCode=-1,
                const std::error_category* category=nullptr
            ) : NativeError(std::move(nativeMessage),nativeCode,category),
                m_prevError(std::move(prevError))
        {}

        //! Ctor
        ErrorStack(
                Error prevError,
                int nativeCode,
                const std::error_category* category=nullptr
            ) : NativeError(nativeCode,category),
                m_prevError(std::move(prevError))
        {}

        //! Ctor
        ErrorStack(
                Error prevError,
                const std::error_category* category
            ) : NativeError(category),
                m_prevError(std::move(prevError))
        {}

        ~ErrorStack();
        ErrorStack(const ErrorStack&)=default;
        ErrorStack(ErrorStack&&) =default;
        ErrorStack& operator=(const ErrorStack&)=default;
        ErrorStack& operator=(ErrorStack&&) =default;

        const Error& prevError() const noexcept
        {
            return m_prevError;
        }

        //! Get native error message.
        virtual std::string nativeMessage() const override
        {
            auto msg=NativeError::nativeMessage();
            if (msg.empty())
            {
                return m_prevError.message();
            }
            return fmt::format("{}: {}",msg,m_prevError.message());
        }

    private:

        Error m_prevError;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNERRORSTACK_H
