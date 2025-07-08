/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/autherror.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHERROR_H
#define HATNAPIAUTHERROR_H

#include <hatn/common/apierror.h>

#include <hatn/api/api.h>

#define HATN_API_AUTH_ERRORS(Do) \
    Do(ApiAuthError,OK,_TR("OK")) \
    Do(ApiAuthError,AUTH_PROTOCOL_UNKNOWN,_TR("unknown authentication protocol","api")) \
    Do(ApiAuthError,AUTH_MISSING,_TR("missing authentication in request","api")) \
    Do(ApiAuthError,AUTH_HEADER_FORMAT,_TR("invalid format of authentication header","api")) \
    Do(ApiAuthError,AUTH_NEGOTIATION_FAILED,_TR("failed to negotiate authentication protocol","api")) \
    Do(ApiAuthError,AUTH_FAILED,_TR("authentication failed","api")) \
    Do(ApiAuthError,AUTH_SESSION_INVALID,_TR("authentication session is not valid","api")) \
    Do(ApiAuthError,AUTH_PROCESSING_FAILED,_TR("failed to process authentication","api")) \
    Do(ApiAuthError,AUTH_COMPLETION_FAILED,_TR("failed to complete authentication","api")) \
    Do(ApiAuthError,INVALID_LOGIN_FORMAT,_TR("invalid format of login","api")) \
    Do(ApiAuthError,ACCESS_DENIED,_TR("access denied","api")) \

HATN_API_NAMESPACE_BEGIN

//! Error codes of hatnapi lib.
enum class ApiAuthError : int
{
    HATN_API_AUTH_ERRORS(HATN_ERROR_CODE)
};

//! API errors codes as strings.
constexpr const char* const ApiAuthErrorStrings[] = {
    HATN_API_AUTH_ERRORS(HATN_ERROR_STR)
};

//! API error code to string.
inline const char* apiAuthErrorString(ApiAuthError code)
{
    return errorString(code,ApiAuthErrorStrings);
}

//! Error category for hatnapi.
class HATN_API_EXPORT ApiAuthErrorCategory : public common::ApiErrorCategory
{
    public:

        //! Get string representation of the code.
        virtual const char* status(int code) const;

        //! Name of the category.
        virtual const char *family() const noexcept
        {
            return "hatn.api.auth";
        }

        //! Get description for the code.
        virtual std::string message(int code,const common::Translator* translator=nullptr) const;

        //! Get category
        static const ApiAuthErrorCategory& getCategory() noexcept;
};

inline common::ApiError apiAuthError(ApiAuthError code)
{
    return common::ApiError{code,&ApiAuthErrorCategory::getCategory()};
}

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHERROR_H
