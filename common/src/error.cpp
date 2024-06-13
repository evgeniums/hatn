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

ApiError::~ApiError()=default;

//---------------------------------------------------------------

int ApiError::apiCode() const noexcept
{
    if (m_error!=nullptr)
    {
        return m_error->value();
    }
    return 0;
}

//---------------------------------------------------------------

std::string ApiError::apiMessage() const
{
    if (m_error!=nullptr)
    {
        return m_error->message();
    }
    return std::string();
}

//---------------------------------------------------------------

std::string ApiError::apiFamily() const
{
    if (m_error!=nullptr)
    {
        return m_error->category()->name();
    }
    return std::string();
}

/********************** NativeError **************************/

NativeError::~NativeError()=default;

/********************** Error **************************/

//---------------------------------------------------------------

const ApiError* Error::apiError() const noexcept
{
    auto err=native();
    if (err==nullptr)
    {
        return nullptr;
    }
    return err->apiError();
}

//---------------------------------------------------------------

bool Error::compareNative(const Error& other) const noexcept
{
    return *other.native()==*this->native();
}

//---------------------------------------------------------------

const std::error_category* Error::nativeCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    return nativeError->category();
}

//---------------------------------------------------------------

const boost::system::error_category* Error::nativeBoostCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    return nativeError->boostCategory();
}

//---------------------------------------------------------------

std::string Error::nativeMessage(const std::shared_ptr<NativeError>& nativeError) const
{
    auto msg=nativeError->message();
    if (nativeError->category()!=nullptr)
    {
        if (!msg.empty())
        {
            return fmt::format("{}: {}", nativeError->category()->message(m_code), msg);
        }
        return nativeError->category()->message(m_code);
    }
    else if (nativeError->boostCategory()!=nullptr)
    {
        if (!msg.empty())
        {
            return fmt::format("{}: {}", nativeError->boostCategory()->message(m_code), msg);
        }
        return nativeError->boostCategory()->message(m_code);
    }
    return msg;
}

//---------------------------------------------------------------

void Error::stackWith(Error&& next)
{
    auto nextNative=const_cast<NativeError*>(next.native());
    if (nextNative!=nullptr)
    {
        nextNative->setPrevError(std::move(*this));
    }
    else
    {
        auto newNextNative=std::make_shared<NativeError>(next.category());
        newNextNative->setBoostCategory(next.boostCategory());
        newNextNative->setPrevError(std::move(*this));

        next.setNative(next.value(),std::move(newNextNative));
    }
    *this=std::move(next);
}

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
        case (static_cast<int>(CommonError::NOT_FOUND)):
            result=_TR("not found");
            break;
        case (static_cast<int>(CommonError::INVALID_TIME_FORMAT)):
            result=_TR("invalid time format");
            break;
        case (static_cast<int>(CommonError::INVALID_DATE_FORMAT)):
            result=_TR("invalid date format");
            break;
        case (static_cast<int>(CommonError::INVALID_DATETIME_FORMAT)):
            result=_TR("invalid datetime format");
            break;
        case (static_cast<int>(CommonError::INVALID_DATERANGE_FORMAT)):
            result=_TR("invalid format of date range");
            break;

        default:
            result=_TR("unknown error");
    }
    return result;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
