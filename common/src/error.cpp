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

#include <hatn/common/ipp/error.ipp>

HATN_COMMON_NAMESPACE_BEGIN

/********************** ApiErrorCategory **************************/

ApiErrorCategory::~ApiErrorCategory()
{}

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

const ErrorCategory* Error::nativeCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    return nativeError->category();
}

//---------------------------------------------------------------

const std::error_category* Error::nativeSystemCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    return nativeError->systemCategory();
}

//---------------------------------------------------------------

const boost::system::error_category* Error::nativeBoostCategory(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    return nativeError->boostCategory();
}

//---------------------------------------------------------------

int Error::nativeErrorCondition(const std::shared_ptr<NativeError>& nativeError) const noexcept
{
    if (nativeError->prevError()!=nullptr)
    {
        return nativeError->prevError()->errorCondition();
    }
    else if (nativeError->category()!=nullptr)
    {
        return nativeError->category()->default_error_condition(m_code).value();
    }
    else if (nativeError->systemCategory()!=nullptr)
    {
        return nativeError->systemCategory()->default_error_condition(m_code).value();
    }
    else if (nativeError->boostCategory()!=nullptr)
    {
        return nativeError->boostCategory()->default_error_condition(m_code).value();
    }
    return m_code;
}

//---------------------------------------------------------------

void Error::setPrevError(Error &&prev, bool usePrevApiError)
{
    auto n=native();
    if (n==nullptr)
    {
        auto newNative=std::make_shared<NativeError>(category());
        newNative->setBoostCategory(boostCategory());
        newNative->setSystemCategory(systemCategory());
        newNative->setPrevError(std::move(prev));
        setNative(std::move(newNative));
    }
    else
    {
        n->setPrevError(std::move(prev),usePrevApiError);
    }
}

//---------------------------------------------------------------

void Error::stackWith(Error&& next, bool keepThisApiError)
{
    next.setPrevError(std::move(*this),keepThisApiError);
    *this=std::move(next);
}

//---------------------------------------------------------------

template HATN_COMMON_EXPORT void Error::nativeMessage<FmtAllocatedBufferChar>(const std::shared_ptr<NativeError>& nativeError, FmtAllocatedBufferChar& buf) const;
template HATN_COMMON_EXPORT void Error::nativeCodeString<FmtAllocatedBufferChar>(const std::shared_ptr<NativeError>& nativeError, FmtAllocatedBufferChar& buf) const;

/********************** CommonErrorCategory **************************/

//---------------------------------------------------------------
const CommonErrorCategory& CommonErrorCategory::getCategory() noexcept
{
    static CommonErrorCategory inst;
    return inst;
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
const char* CommonErrorCategory::codeString(int code) const
{
    return errorString(code,CommonErrorStrings);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
