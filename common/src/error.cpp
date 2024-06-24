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

void Error::nativeMessage(const std::shared_ptr<NativeError>& nativeError, FmtAllocatedBufferChar& buf) const
{
    if (nativeError->category()!=nullptr)
    {
        fmt::format_to(std::back_inserter(buf),"{}: ", nativeError->category()->message(m_code));
    }
    else if (nativeError->boostCategory()!=nullptr)
    {
        fmt::format_to(std::back_inserter(buf),"{}: ", nativeError->boostCategory()->message(m_code));
    }

    nativeError->message(buf);
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

        HATN_COMMON_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }
    return result;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
