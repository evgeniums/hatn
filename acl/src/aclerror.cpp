/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file acl/aclerror.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/acl/aclerror.h>

HATN_ACL_NAMESPACE_BEGIN

/********************** AclErrorCategory **************************/

//---------------------------------------------------------------
const AclErrorCategory& AclErrorCategory::getCategory() noexcept
{
    static AclErrorCategory AclErrorCategoryInstance;
    return AclErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string AclErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_ACL_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* AclErrorCategory::codeString(int code) const
{
    return errorString(code,AclErrorStrings);
}

//---------------------------------------------------------------

HATN_ACL_NAMESPACE_END
