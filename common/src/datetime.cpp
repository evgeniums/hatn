/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/datetime.cpp
 *
 *     Definitions of date and time types.
 *
 */

#include <chrono>
#include "boost/date_time/local_time/local_time.hpp"

#include <hatn/common/datetime.h>

HATN_COMMON_NAMESPACE_BEGIN

/**************************** Date ******************************/

//---------------------------------------------------------------

Result<Date> Date::parse(const lib::string_view& str)
{
    //! @todo implement
    return Error{CommonError::NOT_IMPLEMENTED};
}

//---------------------------------------------------------------

std::string Date::toString(const lib::string_view& format) const
{
    //! @todo implement
    return std::string{};
}

//---------------------------------------------------------------

Date Date::currentUtc()
{
    //! @todo implement
    return Date{};
}

/**************************** Time ******************************/

//---------------------------------------------------------------

Result<Time> Time::parse(const lib::string_view& format)
{
    //! @todo implement
    return Error{CommonError::NOT_IMPLEMENTED};
}

//---------------------------------------------------------------

std::string Time::toString(const lib::string_view& format) const
{
    //! @todo implement
    return std::string{};
}

//---------------------------------------------------------------

Time Time::currentUtc()
{
    //! @todo implement
    return Time{};
}

/**************************** DateTimeUtc ******************************/

//---------------------------------------------------------------

DateTimeUtc::DateTimeUtc()
{}

//---------------------------------------------------------------

DateTimeUtc::~DateTimeUtc()
{}

//---------------------------------------------------------------

std::string DateTimeUtc::toString(const lib::string_view& format) const
{
    //! @todo implement
    return std::string{};
}

//---------------------------------------------------------------

Result<DateTimeUtc> DateTimeUtc::parse(const lib::string_view& str)
{
    //! @todo implement
    return Error{CommonError::NOT_IMPLEMENTED};
}

//---------------------------------------------------------------

DateTimeUtc DateTimeUtc::currentUtc()
{
    //! @todo implement
    return DateTimeUtc{};
}

//---------------------------------------------------------------

uint64_t DateTimeUtc::msSinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//---------------------------------------------------------------

Result<DateTimeUtc> DateTimeUtc::fromMsSinceEpoch(uint64_t value)
{
    //! @todo implement
    return Error{CommonError::NOT_IMPLEMENTED};
}

/**************************** TimeZone ******************************/

//---------------------------------------------------------------

Error TimeZone::setOffset(int8_t value)
{
    m_offset=static_cast<decltype(m_offset)>(value);
    //! @todo implement name update
    return OK;
}

//---------------------------------------------------------------

Error TimeZone::setName(std::string name)
{
    //! @todo implement
    m_name=std::move(name);
    //! @todo implement offset update
    return CommonError::NOT_IMPLEMENTED;
}

/**************************** DateTime ******************************/

//---------------------------------------------------------------

DateTime::DateTime()
{}

//---------------------------------------------------------------

DateTime::~DateTime()
{}

//---------------------------------------------------------------

std::string DateTime::toString(const lib::string_view& format) const
{
    //! @todo implement
    return std::string{};
}

//---------------------------------------------------------------

Result<DateTime> DateTime::parse(const lib::string_view& str)
{
    //! @todo implement
    return Error{CommonError::NOT_IMPLEMENTED};
}

//---------------------------------------------------------------

DateTime DateTime::currentUtc()
{
    //! @todo implement
    return DateTime{};
}

//---------------------------------------------------------------

DateTime DateTime::currentLocal()
{
    //! @todo implement
    return DateTime{};
}

//---------------------------------------------------------------

DateTime DateTime::toTz(const DateTime& from, TimeZone tz)
{
    //! @todo implement
}

//---------------------------------------------------------------

DateTime DateTime::toTz(const DateTimeUtc& from, TimeZone tz)
{
    //! @todo implement
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
