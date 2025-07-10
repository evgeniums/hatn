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
#include <hatn/common/format.h>

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

        bool is(const ApiErrorCategory& other) const noexcept
        {
            return strcmp(family(),other.family())==0;
        }
};

//! API error is used to hold information to be sent back as a result of some request or API command.
class HATN_COMMON_EXPORT ApiError
{
    public:

        constexpr static const char* DefaultStatus="success";

        template <typename T>
        ApiError(T code=0, const ApiErrorCategory* cat=nullptr)
            : ApiError(static_cast<int>(code),cat)
        {}

        ApiError(int code=0, const ApiErrorCategory* cat=nullptr)
            : m_cat(cat),
              m_code(code),
              m_nestedMessage(false)
        {}

        void setCode(int code) noexcept
        {
            m_code=code;
        }

        int code() const noexcept
        {
            return m_code;
        }

        void setStatus(std::string status)
        {
            m_status=std::move(status);
        }

        const char* status() const noexcept
        {
            if (m_cat==nullptr || !m_status.empty())
            {
                return m_status.c_str();
            }
            return m_cat->status(m_code);
        }

        void setDescription(std::string description, bool nested=false)
        {
            m_description=std::move(description);
            m_nestedMessage=nested;
        }

        std::string message(const Translator* translator=nullptr) const
        {
            if (!m_description.empty())
            {
                if (m_nestedMessage)
                {
                    auto str=m_cat->message(m_code,translator);
                    fmt::format_to(std::back_inserter(str),": {}",m_description);
                    return str;
                }
                return m_description;
            }
            return m_cat->message(m_code,translator);
        }

        void setFamily(std::string family)
        {
            m_family=std::move(family);
        }

        const char* family() const
        {
            if (m_cat==nullptr || !m_family.empty())
            {
                return m_family.c_str();
            }
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

        bool isNull() const noexcept
        {
            return m_code==0;
        }

        template <typename T>
        bool is(T code, const ApiErrorCategory& cat) const noexcept
        {
            return static_cast<int>(code)==m_code && isFamily(cat);
        }

        bool isFamily(const ApiErrorCategory& cat) const noexcept
        {
            return strcmp(cat.family(),category()->family())==0;
        }

        bool isFamily(lib::string_view other) const noexcept
        {
            return other==family();
        }

    private:

        const ApiErrorCategory* m_cat;
        int m_code;
        ByteArrayShared m_data;
        std::string m_dataType;
        std::string m_description;
        std::string m_status;
        std::string m_family;
        bool m_nestedMessage;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
