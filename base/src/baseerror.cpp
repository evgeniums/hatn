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
std::string BaseErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        case (static_cast<int>(BaseError::OK)):
            result=common::CommonErrorCategory::getCategory().message(code);
            break;

        case (static_cast<int>(BaseError::VALUE_NOT_SET)):
            result=common::_TR("Value not set","base");
            break;

        case (static_cast<int>(BaseError::INVALID_TYPE)):
            result=common::_TR("Invalid type","base");
            break;

        case (static_cast<int>(BaseError::RESULT_ERROR)):
            result=common::_TR("Cannot get value of error result","base");
            break;

        case (static_cast<int>(BaseError::RESULT_NOT_ERROR)):
        result=common::_TR("Cannot move not error result","base");
        break;

        case (static_cast<int>(BaseError::STRING_NOT_NUMBER)):
            result=common::_TR("Cannot convert string to number","base");
            break;

        case (static_cast<int>(BaseError::UNSUPPORTED_CONFIG_FORMAT)):
            result=common::_TR("Configuration format not supported","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_PARSE_ERROR)):
            result=common::_TR("Failed to parse configuration file","base");
            break;

        case (static_cast<int>(BaseError::UNKKNOWN_CONFIG_MERGE_MODE)):
            result=common::_TR("Unknown mode for configuration files merging","base");
            break;

        default:
            result=common::_TR("Unknown error");
    }

    return result;
}

HATN_BASE_NAMESPACE_END
