/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/nativeerror.h
  *
  *     Contains definition of base native error.
  *
  */

/****************************************************************************/

#ifndef HATNNATIVEERROR_H
#define HATNNATIVEERROR_H

#include <string>

#include <hatn/common/common.h>
#include <hatn/common/format.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/errorcategory.h>
#include <hatn/common/apierror.h>
#include <hatn/common/error.h>

HATN_COMMON_NAMESPACE_BEGIN

class ByteArray;

//! Base class for native errors.
class HATN_COMMON_EXPORT NativeError
{
    public:

        //! Ctor
        NativeError(
                std::string nativeMessage,
                int nativeCode=-1,
                const ErrorCategory* category=nullptr
            ) : m_nativeMessage(std::move(nativeMessage)),
                m_nativeCode(nativeCode),
                m_category(category),
                m_systemCategory(nullptr),
                m_boostCategory(nullptr)
        {}

        //! Ctor
        NativeError(
                int nativeCode,
                const ErrorCategory* category=nullptr
            ) : m_nativeCode(nativeCode),
                m_category(category),
                m_systemCategory(nullptr),
                m_boostCategory(nullptr)
        {}

        //! Ctor
        NativeError(
                const ErrorCategory* category
            ) : m_nativeCode(-1),
                m_category(category),
                m_systemCategory(nullptr),
                m_boostCategory(nullptr)
        {}

        //! Ctor
        NativeError(
        ) : m_nativeCode(-1),
            m_category(nullptr),
            m_systemCategory(nullptr),
            m_boostCategory(nullptr)
        {}

        virtual ~NativeError();
        NativeError(const NativeError&)=default;
        NativeError(NativeError&&) =default;
        NativeError& operator=(const NativeError&)=default;
        NativeError& operator=(NativeError&&) =default;

        //! Get native error message.
        virtual std::string nativeMessage() const
        {
            return m_nativeMessage;
        }

        //! Get error message.
        std::string message() const
        {
            FmtAllocatedBufferChar buf;
            message(buf);
            return fmtBufToString(buf);
        }

        template <typename BufT>
        void message(BufT& buf) const
        {
            auto msg=nativeMessage();
            if (m_prevError)
            {
                if (msg.empty())
                {
                    m_prevError->message(buf);
                }
                else
                {
                    buf.append(msg);
                    buf.append(lib::string_view(": "));
                    m_prevError->message(buf);
                }
            }
            else
            {
                buf.append(msg);
            }
        }

        template <typename BufT>
        void codeString(BufT& buf) const
        {
            if (m_prevError)
            {
                buf.append(lib::string_view(":"));
                m_prevError->codeString(buf);
            }
        }

        //! Get native error code.
        int nativeCode() const noexcept
        {
            return m_nativeCode;
        }

        //! Get category.
        const ErrorCategory* category() const noexcept
        {
            return m_category;
        }

        void setCategory(const ErrorCategory* cat) noexcept
        {
            m_category=cat;
        }

        //! Get system category.
        const std::error_category* systemCategory() const noexcept
        {
            return m_systemCategory;
        }

        void setSystemCategory(const std::error_category* cat) noexcept
        {
            m_systemCategory=cat;
        }

        const boost::system::error_category* boostCategory() const noexcept
        {
            return m_boostCategory;
        }

        void setBoostCategory(const boost::system::error_category* cat) noexcept
        {
            m_boostCategory=cat;
        }

        //! Compare with other error.
        inline bool isEqual(const NativeError& other) const noexcept
        {
            if (
                other.nativeCode()!=this->nativeCode()
                ||
                other.category()!=this->category()
                ||
                other.systemCategory()!=this->systemCategory()
                ||
                other.boostCategory()!=this->boostCategory()
                ||
                other.nativeMessage()!=this->nativeMessage()
                ||
                !this->compareContent(other)
               )
            {
                return false;
            }
            return true;
        }

        bool operator ==(const NativeError& other) const noexcept
        {
            return isEqual(other);
        }
        bool operator !=(const NativeError& other) const noexcept
        {
            return !isEqual(other);
        }

        void setPrevError(Error&& error)
        {
            m_prevError=std::move(error);
        }

        void setApiError(ApiError error)
        {
            m_apiError=std::move(error);
        }

        const Error* prevError() const noexcept
        {
            if (m_prevError)
            {
                return &(m_prevError.value());
            }
            return nullptr;
        }

        const ApiError* apiError() const noexcept
        {
            if (m_apiError)
            {
                return &(m_apiError.value());
            }
            if (m_prevError)
            {
                return m_prevError->apiError();
            }
            return nullptr;
        }

    protected:

        virtual bool compareContent(const common::NativeError&) const noexcept
        {
            return true;
        }

    private:

        std::string m_nativeMessage;
        int m_nativeCode;
        const ErrorCategory* m_category;
        const std::error_category* m_systemCategory;
        const boost::system::error_category* m_boostCategory;

        lib::optional<Error> m_prevError;
        lib::optional<ApiError> m_apiError;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNNATIVEERROR_H
