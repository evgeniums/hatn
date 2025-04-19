/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file acl/aclerrorcodes.h
  *
  * Contains error codes for hatnacl lib.
  *
  */

/****************************************************************************/

#ifndef HATNACLERRORCODES_H
#define HATNACLERRORCODES_H

#include <hatn/common/error.h>
#include <hatn/acl/acl.h>

#define HATN_ACL_ERRORS(Do) \
    Do(AclError,OK,_TR("OK")) \


HATN_ACL_NAMESPACE_BEGIN

//! Error codes of hatnacl lib.
enum class AclError : int
{
    HATN_ACL_ERRORS(HATN_ERROR_CODE)
};

//! acl errors codes as strings.
constexpr const char* const AclErrorStrings[] = {
    HATN_ACL_ERRORS(HATN_ERROR_STR)
};

//! Acl error code to string.
inline const char* aclErrorString(AclError code)
{
    return errorString(code,AclErrorStrings);
}

HATN_ACL_NAMESPACE_END

#endif // HATNACLERRORCODES_H
