/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/mqerror.h
  *
  * Contains declarations of error helpers for hatnmq lib.
  *
  */

/****************************************************************************/

#ifndef HATNAPILIBERROR_H
#define HATNAPILIBERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/mqerrorcodes.h>

HATN_MQ_NAMESPACE_BEGIN

//! Error category for hatnmq.
class HATN_MQ_EXPORT MqErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.mq";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const MqErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code MqError code.
 * @return Error object.
 */
inline Error mqError(MqError code) noexcept
{
    return Error(static_cast<int>(code),&MqErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code MqError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error mqError(MqError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&MqErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_MQ_NAMESPACE_END

#endif // HATNAPIERROR_H
