/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
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

HATN_COMMON_NAMESPACE_BEGIN

/********************** NativeError **************************/

//---------------------------------------------------------------
Error NativeError::serializeAppend(ByteArray& buf) const
{
    std::ignore=buf;
    return Error();
}

/********************** Error **************************/
Error Error::serialize(ByteArray& buf) const {
    int32_t code=m_code;
    boost::endian::native_to_little_inplace(code);
    buf.resize(sizeof(code));
    memcpy(buf.data(),&code,sizeof(code));
    if (m_native)
    {
        HATN_CHECK_RETURN(m_native->serializeAppend(buf))
    }
    return Error();
}

/********************** CommonErrorCategory **************************/

static CommonErrorCategory CommonErrorCategoryInstance;

//---------------------------------------------------------------
const CommonErrorCategory& CommonErrorCategory::getCategory() noexcept
{
    return CommonErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string CommonErrorCategory::message(int code, const std::string& nativeMessage) const
{
    std::string result=nativeMessage;
    if (result.empty())
    {
        switch (code)
        {
            case (static_cast<int>(CommonError::OK)):
                result=_TR("OK");
            break;
            case (static_cast<int>(CommonError::INVALID_SIZE)):
                result=_TR("Invalid size");
            break;
            case (static_cast<int>(CommonError::INVALID_ARGUMENT)):
                result=_TR("Invalid argument");
            break;
            case (static_cast<int>(CommonError::UNSUPPORTED)):
                result=_TR("Operation is not supported");
            break;
            case (static_cast<int>(CommonError::INVALID_FILENAME)):
                result=_TR("Invalide file name");
            break;
            case (static_cast<int>(CommonError::FILE_FLUSH_FAILED)):
                result=_TR("Failed to flush file");
            break;
            case (static_cast<int>(CommonError::FILE_ALREADY_OPEN)):
                result=_TR("File is already open");
            break;
            case (static_cast<int>(CommonError::FILE_WRITE_FAILED)):
                result=_TR("Failed to write file");
            break;
            case (static_cast<int>(CommonError::FILE_NOT_OPEN)):
                result=_TR("File not open");
            break;
            case (static_cast<int>(CommonError::TIMEOUT)):
                result=_TR("Operation timeout");
            break;

            default:
                result=_TR("Unknown error");
        }
    }
    return result;
}

/********************** BoostErrorCategory **************************/

static BoostErrorCategory BoostErrorCategoryInstance;

//---------------------------------------------------------------
const BoostErrorCategory& BoostErrorCategory::getCategory() noexcept
{
    return BoostErrorCategoryInstance;
}

/********************** SystemErrorCategory **************************/

static SystemErrorCategory SystemErrorCategoryInstance;

//---------------------------------------------------------------
const SystemErrorCategory& SystemErrorCategory::getCategory() noexcept
{
    return SystemErrorCategoryInstance;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
