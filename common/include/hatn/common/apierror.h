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
#include <hatn/common/bytearray.h>

HATN_COMMON_NAMESPACE_BEGIN

class Error;
class Translator;

class HATN_COMMON_EXPORT ApiErrorCategory
{
    public:

        virtual ~ApiErrorCategory();

        //! Get string representation of the code.
        virtual const char* status(int code) const=0;

        //! Name of the category.
        virtual const char *family() const noexcept=0;

        //! Get description for the code.
        virtual std::string message(int code,const Translator* translator=nullptr) const=0;
};

//! API error is used to hold information to be sent back as a result of some request or API command.
class HATN_COMMON_EXPORT ApiError
{
    public:

        constexpr static const char* DefaultStatus="success";

        ApiError(int code, const ApiErrorCategory* cat)
            : m_cat(cat),
              m_code(code)
        {}

        void setCode(int code) noexcept
        {
            m_code=code;
        }

        int code() const noexcept
        {
            return m_code;
        }

        const char* status() const noexcept
        {
            return m_cat->status(m_code);
        }

        std::string message(const Translator* translator=nullptr) const
        {
            return m_cat->message(m_code,translator);
        }

        const char* family() const
        {
            return m_cat->family();
        }

        const auto* category() const
        {
            return m_cat;
        }

        void setData(ByteArrayShared data)
        {
            m_data=std::move(data);
        }

        ByteArrayShared data() const
        {
            return m_data;
        }

        void setDataType(std::string type)
        {
            m_dataType=std::move(type);
        }

        const std::string& dataType() const noexcept
        {
            return m_dataType;
        }

    private:

        const ApiErrorCategory* m_cat;
        int m_code;
        ByteArrayShared m_data;
        std::string m_dataType;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
