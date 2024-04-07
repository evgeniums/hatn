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
    if (!val)
    {
        threadLocal().reset();
    }
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
    //! @todo Make in-function static for all categories.
    static DataunitErrorCategory inst;
    return inst;
}

//---------------------------------------------------------------
std::string DataunitErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
    case (static_cast<int>(UnitError::OK)):
        result=common::CommonErrorCategory::getCategory().message(code);
        break;

    case (static_cast<int>(UnitError::PARSE_ERROR)):
        result=_TR("failed to parse object","dataunit");
        break;

    case (static_cast<int>(UnitError::SERIALIZE_ERROR)):
        result=_TR("failed to serialize object","dataunit");
        break;

    default:
        result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
