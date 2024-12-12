/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/datauniterror.сpp
  *
  * Defines dataunit error classes.
  *
  */

#include <hatn/common/translate.h>

#include <hatn/dataunit/datauniterror.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/********************** RawError **************************/

//---------------------------------------------------------------

RawError& RawError::threadLocal() noexcept
{
    static thread_local RawError inst;
    return inst;
}

bool& RawError::enablingTL() noexcept
{
    static thread_local bool enabled;
    return enabled;
}

bool RawError::isEnabledTL() noexcept
{
    return enablingTL();
}

void RawError::setEnabledTL(bool val) noexcept
{
    enablingTL()=val;
    threadLocal().reset();
}

/********************** UnitNativeError **************************/

//---------------------------------------------------------------
UnitNativeError::UnitNativeError(
        const RawError &rawError
    ) : common::NativeError(rawError.message,static_cast<int>(rawError.code),&DataunitErrorCategory::getCategory()),
        m_field(rawError.field)
{}

/********************** DataunitErrorCategory **************************/

//---------------------------------------------------------------
const DataunitErrorCategory& DataunitErrorCategory::getCategory() noexcept
{
    static DataunitErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string DataunitErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {

        HATN_DATAUNIT_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* DataunitErrorCategory::codeString(int code) const
{
    return errorString(code,UnitErrorStrings);
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
