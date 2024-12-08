/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/networkerror.h
  *
  *  Error classes for Hatn Network Library.
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKERROR_H
#define HATNNETWORKERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

#include <hatn/network/networkerrorcodes.h>
#include <hatn/network/network.h>

HATN_NETWORK_NAMESPACE_BEGIN

//! Network error category
class HATN_NETWORK_EXPORT NetworkErrorCategory : public common::ErrorCategory
{
    public:

        //! Name of the category
        virtual const char *name() const noexcept
        {
            return "hatn.network";
        }

        //! Get description for the code
        virtual std::string message(int code) const;

        //! Get string representation of the code.
        virtual const char* codeString(int code) const;

        //! Get category
        static const NetworkErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code Error code.
 * @return Error object.
 */
inline Error networkError(NetworkError code) noexcept
{
    return Error(code,&NetworkErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code Error code.
 * @param native Native error.
 * @return Error object.
 */
inline Error networkError(NetworkError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&NetworkErrorCategory::getCategory());
    return Error(code,std::move(err));
}

/**
 * @brief Make ErrorResult from dbError
 * @param code Error code
 * @return ErrorResult
 */
inline ErrorResult networkErrorResult(NetworkError code) noexcept
{
    return ErrorResult{networkError(code)};
}

inline void setNetworkErrorCode(Error& ec, NetworkError code)
{
    ec.setCode(code,&NetworkErrorCategory::getCategory());
}

HATN_NETWORK_NAMESPACE_END

#endif // HATNNETWORKERROR_H
