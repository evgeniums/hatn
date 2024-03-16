/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/error.сpp
  *
  *      Contains definition of error category;
  *
  */

#include <hatn/common/translate.h>

#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

/********************** BaseErrorCategory **************************/

static BaseErrorCategory BaseErrorCategoryInstance;

//---------------------------------------------------------------
const BaseErrorCategory& BaseErrorCategory::getCategory() noexcept
{
    return BaseErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string BaseErrorCategory::message(int code, const std::string& nativeMessage) const
{
    std::string result=nativeMessage;
    if (result.empty())
    {
        switch (code)
        {
        case (static_cast<int>(ErrorCode::OK)):
            result=common::CommonErrorCategory::getCategory().message(code);
            break;

        case (static_cast<int>(ErrorCode::VALUE_NOT_SET)):
            result=common::_TR("Value not set","base");
            break;

        case (static_cast<int>(ErrorCode::INVALID_TYPE)):
            result=common::_TR("Invalid type","base");
            break;

        default:
            result=common::_TR("Unknown error");
        }
    }
    return result;
}

HATN_BASE_NAMESPACE_END
