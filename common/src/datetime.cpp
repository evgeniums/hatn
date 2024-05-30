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
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn/common/datetime.h>

HATN_COMMON_NAMESPACE_BEGIN

/**************************** Date ******************************/

//---------------------------------------------------------------

Result<Date> Date::parse(const lib::string_view& str, Format format, uint16_t baseShortYear)
{
    try
    {
        Date dt;

        if (format==Format::Number)
        {
            std::string s(str);
            auto num=std::stoi(s);
            if (num<0)
            {
                Error{CommonError::INVALID_DATE_FORMAT};
            }
            HATN_CHECK_RETURN(dt.set(num))
            return dt;
        }
        else
        {
            size_t yearPos=0;
            size_t monthPos=1;
            size_t dayPos=2;
            std::string delimiter{"-"};
            bool shortYear=false;

#if defined(__GNUC__) && __cplusplus < 201703L
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wimplicit-fallthrough\"")
#endif
            switch (format)
            {
                case(Format::IsoSlash):
                    delimiter="/";
                    break;

                case(Format::EuropeShortDot):
                    shortYear=true;
                    HATN_FALLTHROUGH
                case(Format::EuropeDot):
                    yearPos=2;
                    monthPos=1;
                    dayPos=0;
                    delimiter=".";
                    break;

                case(Format::UsShortDot):
                    shortYear=true;
                    HATN_FALLTHROUGH
                case(Format::UsDot):
                    yearPos=2;
                    monthPos=0;
                    dayPos=1;
                    delimiter=".";
                    break;

                case(Format::EuropeShortSlash):
                    shortYear=true;
                    HATN_FALLTHROUGH
                case(Format::EuropeSlash):
                    yearPos=2;
                    monthPos=1;
                    dayPos=0;
                    delimiter="/";
                    break;

                case(Format::UsShortSlash):
                    shortYear=true;
                    HATN_FALLTHROUGH
                case(Format::UsSlash):
                    yearPos=2;
                    monthPos=0;
                    dayPos=1;
                    delimiter="/";
                    break;

                case(Format::Iso):
                case(Format::Number):
                    break;
            }
#if defined(__GNUC__) && __cplusplus < 201703L
    _Pragma("GCC diagnostic pop")
#endif
            std::vector<std::string> parts;
            boost::split(parts,str,boost::is_any_of(delimiter));
            if (parts.size()!=3)
            {
                return Error{CommonError::INVALID_DATE_FORMAT};
            }
            for (size_t i=0;i<3;i++)
            {
                auto val=std::stoi(parts[i]);
                if (val<0)
                {
                    return Error{CommonError::INVALID_DATE_FORMAT};
                }
                if (i==yearPos)
                {
                    if (shortYear)
                    {
                        val=baseShortYear+val;
                    }
                    dt.m_year=static_cast<decltype(dt.m_year)>(val);
                }
                else if (i==monthPos)
                {
                    dt.m_month=static_cast<decltype(dt.m_month)>(val);
                }
                else if (i==dayPos)
                {
                    dt.m_day=static_cast<decltype(dt.m_day)>(val);
                }
            }
            HATN_CHECK_RETURN(dt.validate())
            return dt;
        }
    }
    catch (...)
    {
    }

    return Error{CommonError::INVALID_DATE_FORMAT};
}

//---------------------------------------------------------------

std::string Date::toString(Format format) const
{
    std::string str="invalid";
    if (!isValid())
    {
        return str;
    }

    auto shortYear=[this]()
    {
        return m_year%100;
    };

    switch (format)
    {
        case(Format::Number):
            str=fmt::format("{:04d}{:02d}{:02d}",year(),month(),day());
            break;

        case(Format::Iso):
            str=fmt::format("{:04d}-{:02d}-{:02d}",year(),month(),day());
            break;

        case(Format::IsoSlash):
            str=fmt::format("{:04d}/{:02d}/{:02d}",year(),month(),day());
            break;

        case(Format::EuropeDot):
            str=fmt::format("{:02d}.{:02d}.{:04d}",day(),month(),year());
            break;

        case(Format::UsDot):
            str=fmt::format("{:02d}.{:02d}.{:04d}",month(),day(),year());
            break;

        case(Format::EuropeShortDot):
            str=fmt::format("{:02d}.{:02d}.{:02d}",day(),month(),shortYear());
            break;

        case(Format::UsShortDot):
            str=fmt::format("{:02d}.{:02d}.{:02d}",month(),day(),shortYear());
            break;

        case(Format::EuropeSlash):
            str=fmt::format("{:02d}/{:02d}/{:04d}",day(),month(),year());
            break;

        case(Format::UsSlash):
            str=fmt::format("{:02d}/{:02d}/{:04d}",month(),day(),year());
            break;

        case(Format::EuropeShortSlash):
            str=fmt::format("{:02d}/{:02d}/{:02d}",day(),month(),shortYear());
            break;

        case(Format::UsShortSlash):
            str=fmt::format("{:02d}/{:02d}/{:02d}",month(),day(),shortYear());
            break;

        default:
            str=fmt::format("{:04d}{:02d}{:02d}",year(),month(),day());
            break;
    }

    return str;
}

//---------------------------------------------------------------

Date makeDate(const boost::gregorian::date& d)
{
    return Date{d.year(),d.month(),d.day()};
}

Date Date::currentUtc()
{
    auto d=boost::gregorian::day_clock::universal_day();
    return makeDate(d);
}

//---------------------------------------------------------------

Date Date::currentLocal()
{
    auto d=boost::gregorian::day_clock::local_day();
    return makeDate(d);
}

/**************************** Time ******************************/

//---------------------------------------------------------------

Result<Time> Time::parse(const lib::string_view& str, FormatPrecision precision, bool ampm)
{
    try
    {
        int hour{0};
        int minute{0};
        int second{0};
        int millisecond{0};
        std::string ampmStr;

        std::vector<std::string> parts;
        if (ampm)
        {
            boost::split(parts,str,boost::is_any_of(": "));
        }
        else
        {
            boost::split(parts,str,boost::is_any_of(":."));
        }
        switch (precision)
        {
        case (FormatPrecision::Second):
            if (parts.size()!=(ampm?4:3))
            {
                return Error{CommonError::INVALID_TIME_FORMAT};
            }
            hour=std::stoi(parts[0]);
            minute=std::stoi(parts[1]);
            second=std::stoi(parts[2]);
            if (ampm)
            {
                ampmStr=parts[3];
            }
            break;

        case (FormatPrecision::Minute):
            if (parts.size()!=(ampm?3:2))
            {
                return Error{CommonError::INVALID_TIME_FORMAT};
            }
            hour=std::stoi(parts[0]);
            minute=std::stoi(parts[1]);
            if (ampm)
            {
                ampmStr=parts[2];
            }
            break;

        case (FormatPrecision::Millisecond):
            Assert(!ampm,"Milliseconds not supported in AM/PM mode");
            if (parts.size()!=4)
            {
                return Error{CommonError::INVALID_TIME_FORMAT};
            }
            hour=std::stoi(parts[0]);
            minute=std::stoi(parts[1]);
            second=std::stoi(parts[2]);
            millisecond=std::stoi(parts[3]);
            break;
        }

        if (ampm)
        {
            if (ampmStr=="p.m." || ampmStr=="pm")
            {
                if (hour!=12)
                {
                    hour=hour+12;
                }
            }
            else if (ampmStr=="a.m." || ampmStr=="am")
            {
                if (hour==12)
                {
                    hour=0;
                }
            }
            else
            {
                return Error{CommonError::INVALID_TIME_FORMAT};
            }
        }

        HATN_CHECK_RESULT(validate(hour,minute,second,millisecond))
        return Time{hour,minute,second,millisecond};
    }
    catch (...)
    {
    }

    return Error{CommonError::INVALID_TIME_FORMAT};
}

//---------------------------------------------------------------

std::string Time::toString(FormatPrecision precision, bool ampm) const
{
    std::string str="invalid";
    if (!isValid())
    {
        return str;
    }

    if (ampm)
    {
        std::string ampmStr="a.m.";
        auto hour=m_hour;
        if (m_hour>=12)
        {
            ampmStr="p.m.";
            hour=m_hour-12;
        }
        else if (m_hour==0)
        {
            ampmStr="a.m.";
        }
        if (hour==0)
        {
            hour=12;
        }

        switch (precision)
        {
            case (FormatPrecision::Second):
                str=fmt::format("{}:{:02d}:{:02d} {}",hour,m_minute,m_second,ampmStr);
                break;

            case (FormatPrecision::Minute):
                str=fmt::format("{}:{:02d} {}",hour,m_minute,ampmStr);
                break;

            case (FormatPrecision::Millisecond):
                str=fmt::format("{}:{:02d}:{:02d}.{:03d} {}",hour,m_minute,m_second,m_millisecond,ampmStr);
                break;
        }
    }
    else
    {
        switch (precision)
        {
        case (FormatPrecision::Second):
            str=fmt::format("{:02d}:{:02d}:{:02d}",m_hour,m_minute,m_second);
            break;

        case (FormatPrecision::Minute):
            str=fmt::format("{:02d}:{:02d}",m_hour,m_minute);
            break;

        case (FormatPrecision::Millisecond):
            str=fmt::format("{:02d}:{:02d}:{:02d}.{:03d}",m_hour,m_minute,m_second,m_millisecond);
            break;
        }
    }

    return str;
}

//---------------------------------------------------------------

Time makeTime(const boost::posix_time::ptime& pt)
{
    boost::posix_time::time_duration dt=pt.time_of_day();
    auto ms=dt.total_milliseconds()-dt.total_seconds()*1000;
    return Time{dt.hours(),dt.minutes(),dt.seconds(),ms};
}

Time Time::currentUtc()
{
    auto pt=boost::posix_time::microsec_clock::universal_time();
    return makeTime(pt);
}

//---------------------------------------------------------------

Time Time::currentLocal()
{
    auto pt=boost::posix_time::microsec_clock::local_time();
    return makeTime(pt);
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
    std::ignore=format;
    return std::string{};
}

//---------------------------------------------------------------

Result<DateTimeUtc> DateTimeUtc::parse(const lib::string_view& format)
{
    //! @todo implement
    std::ignore=format;
    return Error{CommonError::NOT_IMPLEMENTED};
}

//---------------------------------------------------------------

DateTimeUtc DateTimeUtc::currentUtc()
{
    auto pt=boost::posix_time::microsec_clock::universal_time();
    return DateTimeUtc{makeDate(pt.date()),makeTime(pt)};
}

//---------------------------------------------------------------

uint64_t DateTimeUtc::msSinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//---------------------------------------------------------------

Result<DateTimeUtc> DateTimeUtc::fromMsSinceEpoch(uint64_t value)
{
    auto seconds=value/1000;
    auto ms=value-seconds;

    auto pt=boost::posix_time::from_time_t(seconds);
    auto date=pt.date();
    auto dt=pt.time_of_day();

    return DateTimeUtc{
        Date{date.year(),date.month(),date.day()},
        Time{dt.hours(),dt.minutes(),dt.seconds(),ms}
    };
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
    std::ignore=format;
    return std::string{};
}

//---------------------------------------------------------------

Result<DateTime> DateTime::parse(const lib::string_view& format)
{
    //! @todo implement
    std::ignore=format;
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
    std::ignore=from;
    std::ignore=tz;
    return DateTime{};
}

//---------------------------------------------------------------

DateTime DateTime::toTz(const DateTimeUtc& from, TimeZone tz)
{
    //! @todo implement
    std::ignore=from;
    std::ignore=tz;
    return DateTime{};
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
