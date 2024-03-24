/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/error.cpp
  *
  *  Base error classes.
  *
  */

/****************************************************************************/

#include <boost/endian/conversion.hpp>

#include <hatn/common/bytearray.h>
#include <hatn/common/translate.h>
#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>
#include <hatn/common/errorstack.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** NativeError **************************/

//---------------------------------------------------------------

NativeError::~NativeError()=default;

/********************** ErrorStack **************************/

//---------------------------------------------------------------

ErrorStack::~ErrorStack()=default;

/********************** CommonErrorCategory **************************/

static CommonErrorCategory CommonErrorCategoryInstance;

//---------------------------------------------------------------
const CommonErrorCategory& CommonErrorCategory::getCategory() noexcept
{
    return CommonErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string CommonErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        case (static_cast<int>(CommonError::OK)):
            result=_TR("OK");
        break;
        case (static_cast<int>(CommonError::INVALID_SIZE)):
            result=_TR("invalid size");
        break;
        case (static_cast<int>(CommonError::INVALID_ARGUMENT)):
            result=_TR("invalid argument");
        break;
        case (static_cast<int>(CommonError::UNSUPPORTED)):
            result=_TR("operation is not supported");
        break;
        case (static_cast<int>(CommonError::INVALID_FILENAME)):
            result=_TR("invalide file name");
        break;
        case (static_cast<int>(CommonError::FILE_FLUSH_FAILED)):
            result=_TR("failed to flush file");
        break;
        case (static_cast<int>(CommonError::FILE_ALREADY_OPEN)):
            result=_TR("file is already open");
        break;
        case (static_cast<int>(CommonError::FILE_WRITE_FAILED)):
            result=_TR("failed to write file");
        break;
        case (static_cast<int>(CommonError::FILE_NOT_OPEN)):
            result=_TR("file not open");
        break;
        case (static_cast<int>(CommonError::TIMEOUT)):
            result=_TR("operation timeout");
        break;
        case (static_cast<int>(CommonError::NOT_IMPLEMENTED)):
            result=_TR("requested operation with provided arguments not implemented yet");
            break;
        case (static_cast<int>(CommonError::RESULT_ERROR)):
            result=_TR("cannot get value of error result");
            break;

        case (static_cast<int>(CommonError::RESULT_NOT_ERROR)):
            result=_TR("cannot move not error result");
            break;

        default:
            result=_TR("unknown error");
    }
    return result;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
