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
#include <hatn/common/format.h>
#include <hatn/common/sharedptr.h>

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

class ByteArrayManaged;

//! What a client should do about an ApiError - the terminal/retryable distinction stated by the
//! server, per whitemdesktop/docs/error-contract.md. Mirrors evgo's generic_error.Disposition
//! string values one-for-one so the wire encoding (x-hatn-edisposition et al) round-trips.
enum class ApiErrorDisposition : uint8_t
{
    //! Server did not state a disposition (absent field, or a peer predating this contract).
    //! The zero value - a client must fall back to its own heuristics.
    Unknown=0,
    //! This request will never succeed as issued.
    Permanent,
    //! The server does not implement this call, or the API version is too old. Terminal like
    //! Permanent, but distinct: a client should stop offering the feature, not just fail this
    //! one call.
    Unsupported,
    //! Transient; retry with backoff.
    Retry,
    //! Retryable, but not yet - see ApiError::retryAfter() for the delay in seconds.
    RetryAfter,
    //! Retryable only after the user does something: re-auth, free storage, raise a quota.
    UserAction
};

inline const char* apiErrorDispositionString(ApiErrorDisposition disposition) noexcept
{
    switch (disposition)
    {
        case (ApiErrorDisposition::Permanent): return "permanent";
        case (ApiErrorDisposition::Unsupported): return "unsupported";
        case (ApiErrorDisposition::Retry): return "retry";
        case (ApiErrorDisposition::RetryAfter): return "retry_after";
        case (ApiErrorDisposition::UserAction): return "user_action";
        case (ApiErrorDisposition::Unknown): break;
    }
    return "";
}

inline ApiErrorDisposition apiErrorDispositionFromString(lib::string_view value) noexcept
{
    if (value=="permanent") return ApiErrorDisposition::Permanent;
    if (value=="unsupported") return ApiErrorDisposition::Unsupported;
    if (value=="retry") return ApiErrorDisposition::Retry;
    if (value=="retry_after") return ApiErrorDisposition::RetryAfter;
    if (value=="user_action") return ApiErrorDisposition::UserAction;
    return ApiErrorDisposition::Unknown;
}

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
            std::string str;

            // m_cat is nullptr for every wire-parsed ApiError (the client reconstructs one from
            // a response's code/status/family/description fields, never from a category), so
            // the m_cat->... calls below must not run unguarded - nested/fallback branches
            // degrade to the plain field instead of dereferencing a null category.

            if (!m_description.empty())
            {
                if (m_nestedMessage && m_cat!=nullptr)
                {
                    str=m_cat->message(m_code,translator);
                    fmt::format_to(std::back_inserter(str),": {}",m_description);
                    return str;
                }

                return m_description;
            }
            else if (!m_status.empty())
            {
                if (m_nestedMessage && m_cat!=nullptr)
                {
                    str=m_cat->message(m_code,translator);
                    fmt::format_to(std::back_inserter(str),": {}",m_status);
                    return str;
                }
                return m_status;
            }

            if (m_cat==nullptr)
            {
                return m_stringCode;
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

        template <typename T>
        void setData(T data)
        {
            m_data=std::move(data);
        }

        auto data() const
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
            return m_code==0 && m_stringCode.empty();
        }

        template <typename T>
        bool is(T code, const ApiErrorCategory& cat) const noexcept
        {
            return static_cast<int>(code)==m_code && isFamily(cat);
        }

        //! Match against a server-authored string code (evgo generic_error.Code), e.g.
        //! apiErr->is("file_content_gone"). See whitemdesktop/docs/error-contract.md.
        bool is(lib::string_view stringCode) const noexcept
        {
            return !stringCode.empty() && stringCode==m_stringCode;
        }

        bool isFamily(const ApiErrorCategory& cat) const noexcept
        {
            return isFamily(cat.family());
        }

        bool isFamily(lib::string_view other) const noexcept
        {
            return other==family();
        }

        void setStringCode(std::string code)
        {
            m_stringCode=std::move(code);
        }

        const std::string& stringCode() const noexcept
        {
            return m_stringCode;
        }

        void setDetails(std::string details)
        {
            m_details=std::move(details);
        }

        const std::string& details() const noexcept
        {
            return m_details;
        }

        void setDisposition(ApiErrorDisposition disposition) noexcept
        {
            m_disposition=disposition;
        }

        ApiErrorDisposition disposition() const noexcept
        {
            return m_disposition;
        }

        void setRetryAfter(int seconds) noexcept
        {
            m_retryAfter=seconds;
        }

        int retryAfter() const noexcept
        {
            return m_retryAfter;
        }

        //! This request will never succeed as issued / this call is unsupported - a client must
        //! not retry it automatically. False (not just "no") when the server said nothing -
        //! callers that need to distinguish must check isStated() first.
        bool isTerminal() const noexcept
        {
            return m_disposition==ApiErrorDisposition::Permanent
                   || m_disposition==ApiErrorDisposition::Unsupported;
        }

        //! Worth retrying (possibly after a delay or a user action) - the mirror of
        //! isTerminal(), and likewise false while isStated() is false.
        bool isRetryable() const noexcept
        {
            return m_disposition==ApiErrorDisposition::Retry
                   || m_disposition==ApiErrorDisposition::RetryAfter
                   || m_disposition==ApiErrorDisposition::UserAction;
        }

        //! Whether the server expressed a disposition opinion at all. When false, neither
        //! isTerminal() nor isRetryable() carries any information - callers fall back to
        //! whatever heuristic they used before this contract existed.
        bool isStated() const noexcept
        {
            return m_disposition!=ApiErrorDisposition::Unknown;
        }

    private:

        const ApiErrorCategory* m_cat;
        int m_code;
        EmbeddedSharedPtr<ByteArrayManaged> m_data;
        std::string m_dataType;
        std::string m_description;
        std::string m_status;
        std::string m_family;
        bool m_nestedMessage;

        std::string m_stringCode;
        std::string m_details;
        ApiErrorDisposition m_disposition=ApiErrorDisposition::Unknown;
        int m_retryAfter=0;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERROR_H
