/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file acl/aclerror.h
  *
  * Contains declarations of error helpers for hatnacl lib.
  *
  */

/****************************************************************************/

#ifndef HATNACLERROR_H
#define HATNACLERROR_H

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/result.h>

#include <hatn/acl/acl.h>
#include <hatn/acl/aclerrorcodes.h>

HATN_ACL_NAMESPACE_BEGIN

//! Error category for hatnacl.
class HATN_ACL_EXPORT AclErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.acl";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

    //! Get category
    static const AclErrorCategory& getCategory() noexcept;
};

/**
 * @brief Make error object from code.
 * @param code AclError code.
 * @return Error object.
 */
inline Error aclError(AclError code) noexcept
{
    return Error(static_cast<int>(code),&AclErrorCategory::getCategory());
}

/**
 * @brief Make error object from code and native error.
 * @param code AclError code.
 * @param native Native error.
 * @return Error object.
 */
inline Error aclError(AclError code, std::shared_ptr<common::NativeError> err) noexcept
{
    err->setCategory(&AclErrorCategory::getCategory());
    return Error(static_cast<int>(code),std::move(err));
}

HATN_ACL_NAMESPACE_END

#endif // HATNACLERROR_H
