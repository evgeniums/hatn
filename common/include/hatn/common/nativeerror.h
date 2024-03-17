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
#include <hatn/common/errorcategory.h>

HATN_COMMON_NAMESPACE_BEGIN

class Error;
class ByteArray;

//! Base class for native errors
class HATN_COMMON_EXPORT NativeError
{
    public:

        //! Ctor
        NativeError(
                int value,
                const ErrorCategory* category=&CommonErrorCategory::getCategory(),
                bool notNull=true
            ) : m_value(value),
                m_category(category),
                m_notNull(notNull)
        {}

        virtual ~NativeError()=default;
        NativeError(const NativeError&)=default;
        NativeError(NativeError&&) =default;
        NativeError& operator=(const NativeError&)=default;
        NativeError& operator=(NativeError&&) =default;

        //! Native message
        virtual std::string nativeMessage() const
        {
            return std::string();
        }

        //! Message
        std::string message() const
        {
            if (m_category!=nullptr)
            {
                return m_category->message(m_value,nativeMessage());
            }
            return nativeMessage();
        }

        //! Code
        int value() const noexcept
        {
            return m_value;
        }

        //! Check if error is NULL
        bool isNull() const noexcept
        {
            return !m_notNull;
        }

        //! Bool operator
        inline operator bool() const noexcept
        {
            return !isNull();
        }

        //! Get category
        const ErrorCategory* getCategory() const noexcept
        {
            return m_category;
        }

        //! Compare with other error
        inline bool isEqual(const NativeError& other) const noexcept
        {
            if (other.getCategory()!=this->getCategory()
               ||
                other.value()!=this->value()
               )
            {
                return false;
            }
            return compareContent(other);
        }

        bool operator ==(const NativeError& other) const noexcept
        {
            return isEqual(other);
        }
        bool operator !=(const NativeError& other) const noexcept
        {
            return !isEqual(other);
        }

        virtual Error serializeAppend(ByteArray& buf) const;

        void setNotNull(bool val) noexcept
        {
            m_notNull=val;
        }

        void setValue(int val) noexcept
        {
            m_value=val;
        }

    protected:

        //! Compare self content with content of other error
        virtual bool compareContent(const NativeError& other) const noexcept
        {
            std::ignore=other;
            return false;
        }

    private:

        int m_value;
        const ErrorCategory* m_category;
        bool m_notNull;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNNATIVEERROR_H
