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
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn/common/format.h>
#include <hatn/common/datetime.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace {

    Date makeDate(const boost::gregorian::date& d)
    {
        return Date{d.year(),d.month(),d.day()};
    }

    Time makeTime(const boost::posix_time::ptime& pt)
    {
        boost::posix_time::time_duration dt=pt.time_of_day();
        auto ms=dt.total_milliseconds()-dt.total_seconds()*1000;
        return Time{dt.hours(),dt.minutes(),dt.seconds(),ms};
    }

    std::pair<boost::posix_time::ptime,int8_t> utcToLocal(const boost::posix_time::ptime& utc)
    {
        boost::date_time::c_local_adjustor<boost::posix_time::ptime> adj;
        auto localDt=adj.utc_to_local(utc);
        auto tz=localDt-utc;
        return std::make_pair(localDt,static_cast<int8_t>(tz.total_seconds()/60));
    }

    boost::gregorian::date toBoostDate(const Date& dt)
    {
        return boost::gregorian::date{dt.year(),dt.month(),dt.day()};
    }

    boost::posix_time::time_duration toBoostTimeDuration(const Time& t)
    {
        return boost::posix_time::time_duration{t.hour(),t.minute(),t.second(),t.millisecond()};
    }

    boost::posix_time::ptime toBoostPtime(const DateTime& dt)
    {
        return boost::posix_time::ptime{toBoostDate(dt.date()),toBoostTimeDuration(dt.time())};
    }

} // anonymous namespace

/**************************** Date ******************************/

//---------------------------------------------------------------

Result<Date> Date::parse(const lib::string_view& str, Format format, uint16_t baseShortYear)
{
    if (str.empty())
    {
        return Date{};
    }

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
    if (str.empty())
    {
        return Time{};
    }

    auto extractMs=[](const std::string& msStr)
    {
        int millisecond=0;
        switch (msStr.size())
        {
        case(1):
            millisecond=std::stoi(msStr)*100;
            break;
        case(2):
            millisecond=std::stoi(msStr)*10;
            break;
        case(3):
            millisecond=std::stoi(msStr);
            break;
        case(4):
            millisecond=std::stoi(msStr)/10;
            break;
        case(5):
            millisecond=std::stoi(msStr)/100;
            break;
        case(6):
            millisecond=std::stoi(msStr)/1000;
            break;
        default:
            return Result<int>{Error{CommonError::INVALID_TIME_FORMAT}};
        }

        return Result<int>{millisecond};
    };

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
            {
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
            }

            case (FormatPrecision::Minute):
            {
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
            }

            case (FormatPrecision::Millisecond):
            {
                Assert(!ampm,"Milliseconds not supported in AM/PM mode");
                if (parts.size()<3)
                {
                    return Error{CommonError::INVALID_TIME_FORMAT};
                }
                hour=std::stoi(parts[0]);
                minute=std::stoi(parts[1]);
                second=std::stoi(parts[2]);
                if (parts.size()==4)
                {
                    auto r=extractMs(parts[3]);
                    HATN_CHECK_RESULT(r);
                    millisecond=r.takeValue();
                }
                break;
            }

            case (FormatPrecision::Number):
            {
                Assert(!ampm,"Milliseconds not supported in AM/PM mode");
                auto num=std::stoi(parts[0]);
                if (num<0)
                {
                    return Error{CommonError::INVALID_TIME_FORMAT};
                }
                Time t{static_cast<uint64_t>(num)*1000};
                if (parts.size()==2)
                {
                    auto r=extractMs(parts[2]);
                    HATN_CHECK_RESULT(r);
                    millisecond=r.takeValue();
                    HATN_CHECK_RESULT(t.setMillisecond(millisecond))
                }
                return t;
                break;
            }
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

            case (FormatPrecision::Number):
                str=fmt::format("{}{:02d}{:02d}.{:03d} {}",hour,m_minute,m_second,m_millisecond,ampmStr);
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
            if (m_millisecond>0)
            {
                str=fmt::format("{:02d}:{:02d}:{:02d}.{:03d}",m_hour,m_minute,m_second,m_millisecond);
            }
            else
            {
                str=fmt::format("{:02d}:{:02d}:{:02d}",m_hour,m_minute,m_second);
            }
            break;        

        case (FormatPrecision::Number):
            if (m_millisecond>0)
            {
                str=fmt::format("{:02d}{:02d}{:02d}.{:03d}",m_hour,m_minute,m_second,m_millisecond);
            }
            else
            {
                str=fmt::format("{:02d}{:02d}{:02d}",m_hour,m_minute,m_second);
            }
            break;
        }
    }

    return str;
}

//---------------------------------------------------------------

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

/**************************** DateTime ******************************/

//---------------------------------------------------------------

std::string DateTime::toIsoString(bool withMilliseconds) const
{
    FmtAllocatedBufferChar buf;

    fmt::format_to(std::back_inserter(buf),"{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}",
        m_date.year(),m_date.month(),m_date.day(),
        m_time.hour(),m_time.minute(),m_time.second());

    if (withMilliseconds && m_time.millisecond()>0)
    {
        fmt::format_to(std::back_inserter(buf),".{:03d}",m_time.millisecond());
    }

    if (m_tz==0)
    {
        fmt::format_to(std::back_inserter(buf),"Z");
    }
    else
    {
        fmt::format_to(std::back_inserter(buf),"{:+02d}:00",m_tz);
    }

    return fmtBufToString(buf);
}

//---------------------------------------------------------------

Result<DateTime> DateTime::parseIsoString(const lib::string_view& str)
{
    std::vector<std::string> mainParts;
    boost::split(mainParts,str,boost::is_any_of("T"));
    if (mainParts.size()!=2)
    {
        return Error{CommonError::INVALID_DATETIME_FORMAT};
    }

    DateTime dt;

    auto date=Date::parse(mainParts[0]);
    if (date)
    {
        date=Date::parse(mainParts[0],Date::Format::Number);
        HATN_CHECK_RESULT(date)
    }
    dt.m_date=date.takeValue();

    auto parseTime=[&dt](const lib::string_view& str)
    {
        auto time=Time::parse(str,Time::FormatPrecision::Millisecond);
        if (time)
        {
            time=Time::parse(str,Time::FormatPrecision::Number);
            if (time)
            {
                return time.error();
            }
        }
        dt.m_time=time.takeValue();
        return Error{};
    };

    if (boost::algorithm::ends_with(mainParts[1],"Z"))
    {
        mainParts[1].pop_back();
        HATN_CHECK_RETURN(parseTime(mainParts[1]))
    }
    else
    {
        if (mainParts[1].size()<14)
        {
            return Error{CommonError::INVALID_DATETIME_FORMAT};
        }

        auto timeStr=mainParts[1].substr(0,mainParts[1].size()-6);
        HATN_CHECK_RETURN(parseTime(timeStr))

        auto tzStr=mainParts[1].substr(mainParts[1].size()-6,6);
        std::vector<std::string> tzParts;
        boost::split(tzParts,tzStr,boost::is_any_of(":"));
        if (tzParts.size()!=2)
        {
            return Error{CommonError::INVALID_DATETIME_FORMAT};
        }
        try
        {
            auto tz=std::stoi(tzParts[0]);
            HATN_CHECK_RETURN(dt.setTz(tz))
        }
        catch(...)
        {
        }
    }

    return dt;
}

//---------------------------------------------------------------

DateTime DateTime::currentUtc()
{
    auto pt=boost::posix_time::microsec_clock::universal_time();
    return DateTime{makeDate(pt.date()),makeTime(pt),0};
}

//---------------------------------------------------------------

DateTime DateTime::currentLocal()
{
    auto utc=boost::posix_time::microsec_clock::universal_time();
    auto local=utcToLocal(utc);
    const auto& localDt=local.first;

    return DateTime{makeDate(localDt.date()),makeTime(localDt),local.second};
}

//---------------------------------------------------------------

uint64_t DateTime::sinceEpochMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//---------------------------------------------------------------

uint32_t DateTime::sinceEpoch()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//---------------------------------------------------------------

int8_t DateTime::localTz()
{
    auto utc=boost::posix_time::microsec_clock::universal_time();
    auto local=utcToLocal(utc);
    return local.second;
}

//---------------------------------------------------------------

Result<DateTime> DateTime::fromEpochMs(uint64_t value, int8_t tz)
{
    auto seconds=value/1000;
    auto ms=value%1000;
    seconds+=tz*60;

    auto pt=boost::posix_time::from_time_t(seconds);
    auto date=pt.date();
    auto dt=pt.time_of_day();

    return DateTime{
        Date{date.year(),date.month(),date.day()},
        Time{dt.hours(),dt.minutes(),dt.seconds(),ms},
        tz
    };
}

//---------------------------------------------------------------

Result<DateTime> DateTime::fromEpoch(uint32_t value, int8_t tz)
{
    auto seconds=value+tz*60;

    auto pt=boost::posix_time::from_time_t(seconds);
    auto date=pt.date();
    auto dt=pt.time_of_day();

    return DateTime{
        Date{date.year(),date.month(),date.day()},
        Time{dt.hours(),dt.minutes(),dt.seconds(),0},
        tz
    };
}

//---------------------------------------------------------------

Result<DateTime> DateTime::toTz(const DateTime& from, int8_t tz)
{
    HATN_CHECK_RETURN(validateTz(tz))

    if (tz==from.m_tz)
    {
        return from;
    }

    auto diff=tz-from.m_tz;
    auto fromEpoch=from.toEpochMs();
    fromEpoch+=diff*60000;

    return fromEpochMs(fromEpoch,tz);
}

//---------------------------------------------------------------

uint64_t DateTime::toEpochMs() const
{
    auto* utc=this;
    DateTime tmpUtc;
    if (m_tz!=0)
    {
        tmpUtc=toUtc();
        utc=&tmpUtc;
    }

    auto pt=toBoostPtime(*utc);
    auto epoch=boost::posix_time::ptime{boost::gregorian::date{1970,1,1}};
    auto duration=pt-epoch;
    return duration.total_milliseconds();
}

//---------------------------------------------------------------

uint32_t DateTime::toEpoch() const
{
    auto* utc=this;
    DateTime tmpUtc;
    if (m_tz!=0)
    {
        tmpUtc=toUtc();
        utc=&tmpUtc;
    }

    auto pt=toBoostPtime(*utc);
    auto epoch=boost::posix_time::ptime{boost::gregorian::date{1970,1,1}};
    auto duration=pt-epoch;
    return duration.total_seconds();
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
