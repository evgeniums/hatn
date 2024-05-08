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
            result=_TR("value not set","base");
            break;

        case (static_cast<int>(BaseError::INVALID_TYPE)):
            result=_TR("invalid type","base");
            break;

        case (static_cast<int>(BaseError::STRING_NOT_NUMBER)):
            result=_TR("cannot convert string to number","base");
            break;

        case (static_cast<int>(BaseError::UNSUPPORTED_CONFIG_FORMAT)):
            result=_TR("configuration format not supported","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_PARSE_ERROR)):
            result=_TR("failed to parse configuration file","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_LOAD_ERROR)):
            result=_TR("failed to load configuration file","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_SAVE_ERROR)):
            result=_TR("failed to save configuration file","base");
            break;

        case (static_cast<int>(BaseError::UNKKNOWN_CONFIG_MERGE_MODE)):
            result=_TR("unknown merge mode","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_OBJECT_LOAD_ERROR)):
            result=_TR("failed to load configuration object","base");
            break;

        case (static_cast<int>(BaseError::CONFIG_OBJECT_VALIDATE_ERROR)):
            result=_TR("failed to validate configuration object","base");
            break;

        case (static_cast<int>(BaseError::UNSUPPORTED_TYPE)):
            result=_TR("unsupported type","base");
            break;

        default:
            result=_TR("unknown error");
    }

    return result;
}

HATN_BASE_NAMESPACE_END
