/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientServer/clientServererror.h
  *
  * Contains declarations of error helpers for hatnclientServer lib.
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERERROR_H
#define HATNCLIENTSERVERERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/clientservererrorcodes.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//! Error category for hatnclientServer.
class HATN_CLIENT_SERVER_EXPORT ClientServerErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.clientserver";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const ClientServerErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code ClientServerError code.
 * @return Error object.
 */
inline Error clientServerError(ClientServerError code) noexcept
{
    return Error(static_cast<int>(code),&ClientServerErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code ClientServerError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error clientServerError(ClientServerError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&ClientServerErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERERROR_H
