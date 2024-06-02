/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/datetime.h
  *
  * Declarations of date and time types.
  */

/****************************************************************************/

#ifndef HATNDATETIME_H
#define HATNDATETIME_H

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/stdwrappers.h>

HATN_COMMON_NAMESPACE_BEGIN

class DateTime;

class HATN_COMMON_EXPORT Date
{
    public:

        enum class Format : int
        {
            Iso,
            IsoSlash,
            Number,
            EuropeDot,
            UsDot,
            EuropeSlash,
            UsSlash,
            EuropeShortDot,
            UsShortDot,
            EuropeShortSlash,
            UsShortSlash
        };

        Date(): m_year(static_cast<decltype(m_year)>(0)),
                m_month(static_cast<decltype(m_month)>(0)),
                m_day(static_cast<decltype(m_day)>(0))
        {}

        template <typename YearT, typename MonthT, typename DayT>
        Date(YearT year, MonthT month, DayT day)
            : m_year(static_cast<decltype(m_year)>(year)),
              m_month(static_cast<decltype(m_month)>(month)),
              m_day(static_cast<decltype(m_day)>(day))
        {
            HATN_CHECK_THROW(validate())
        }

        Date(uint32_t value)
        {
            HATN_CHECK_THROW(set(value));
        }

        uint16_t year() const noexcept
        {
            return m_year;
        }

        uint8_t month() const noexcept
        {
            return m_month;
        }

        uint8_t day() const noexcept
        {
            return m_day;
        }

        Error set(uint32_t value) noexcept
        {
            auto year=value/10000;
            auto month=(value-year*10000)/100;
            auto day=value%100;
            HATN_CHECK_RETURN(validate(year,month,day))
            m_year=year;
            m_month=month;
            m_day=day;
            return OK;
        }

        uint32_t toNumber() const noexcept
        {
            uint32_t result=m_year*10000 + m_month*100 + m_day;
            return result;
        }

        template <typename T>
        Error setYear(T value) noexcept
        {
            if (value==0)
            {
                return CommonError::INVALID_DATE_FORMAT;
            }
            m_year=static_cast<decltype(m_year)>(value);
            return OK;
        }

        template <typename T>
        Error setMonth(T value) noexcept
        {
            if (value>12 || value==0)
            {
                return CommonError::INVALID_DATE_FORMAT;
            }
            m_month=static_cast<decltype(m_month)>(value);
            return OK;
        }

        template <typename T>
        Error setDay(T value) noexcept
        {
            if (value>31 || value==0)
            {
                return CommonError::INVALID_DATE_FORMAT;
            }
            m_day=static_cast<decltype(m_day)>(value);
            return OK;
        }

        void reset() noexcept
        {
            m_year=static_cast<decltype(m_year)>(0);
            m_month=static_cast<decltype(m_month)>(0);
            m_day=static_cast<decltype(m_day)>(0);
        }

        bool isValid() const noexcept
        {
            return m_year>=1970 || m_month>=1 || m_day>=1;
        }

        bool isNull() const noexcept
        {
            return !isValid();
        }

        static Result<Date> parse(const lib::string_view& str, Format format=Format::Iso, uint16_t baseShortYear=2000);

        std::string toString(Format format=Format::Iso) const;

        static Date currentUtc();

        static Date currentLocal();

        bool operator==(const Date& other) const noexcept
        {
            return m_year==other.m_year && m_month==other.m_month && m_day==other.m_day;
        }

        bool operator<(const Date& other) const noexcept
        {
            return toNumber()<other.toNumber();
        }

        bool operator<=(const Date& other) const noexcept
        {
            return toNumber()<=other.toNumber();
        }

        bool operator>(const Date& other) const noexcept
        {
            return toNumber()>other.toNumber();
        }

        bool operator>=(const Date& other) const noexcept
        {
            return toNumber()>=other.toNumber();
        }

        void addDays(int days);

        uint8_t dayOfWeek() const;

        uint16_t dayOfYear() const;

        uint16_t weekNumber() const;

        static uint16_t daysInMonth(uint16_t year,uint8_t month);

        bool isLeapYear() const;

        static bool isLeapYear(uint16_t year);

    private:

        uint16_t m_year;
        uint8_t m_month;
        uint8_t m_day;

        template <typename YearT, typename MonthT, typename DayT>
        Error validate(YearT year, MonthT month, DayT day) noexcept
        {
            if (month>12 || day>31 || month==0 || day==0 || year==0)
            {
                return CommonError::INVALID_DATE_FORMAT;
            }
            if (day>daysInMonth(static_cast<uint16_t>(year), static_cast<uint8_t>(month)))
            {
                return CommonError::INVALID_DATE_FORMAT;
            }
            return OK;
        }

        Error validate() noexcept
        {
            auto ec=validate(m_year,m_month,m_day);
            if (ec)
            {
                reset();
            }
            return ec;
        }

        friend class DateTime;
};

class HATN_COMMON_EXPORT Time
{
    public:

        enum class FormatPrecision : int
        {
            Minute,
            Second,
            Millisecond,
            Number
        };

        Time(): m_hour(static_cast<decltype(m_hour)>(0)),
                m_minute(static_cast<decltype(m_minute)>(0)),
                m_second(static_cast<decltype(m_second)>(0)),
                m_millisecond(static_cast<decltype(m_millisecond)>(0))
        {}

        template <typename HourT, typename MinuteT, typename SecondT, typename MillisecondT>
        Time(HourT hour, MinuteT minute, SecondT second, MillisecondT ms=0)
            : m_hour(static_cast<decltype(m_hour)>(hour)),
            m_minute(static_cast<decltype(m_minute)>(minute)),
            m_second(static_cast<decltype(m_second)>(second)),
            m_millisecond(static_cast<decltype(m_millisecond)>(ms))
        {
            HATN_CHECK_THROW(validate())
        }

        Time(uint64_t value)
        {
            auto ec=set(value);
            if (ec)
            {
                throw ErrorException{ec};
            }
        }

        uint8_t hour() const noexcept
        {
            return m_hour;
        }

        uint8_t minute() const noexcept
        {
            return m_minute;
        }

        uint8_t second() const noexcept
        {
            return m_second;
        }

        uint16_t millisecond() const noexcept
        {
            return m_millisecond;
        }

        Error set(uint64_t value) noexcept
        {
            auto hour=value/10000000;
            auto minute=(value-hour*10000000)/100000;
            auto second=(value-hour*10000000-minute*100000)/1000;
            auto millisecond=value%1000;

            HATN_CHECK_RETURN(setHour(hour))
            HATN_CHECK_RETURN(setMinute(minute))
            HATN_CHECK_RETURN(setSecond(second))
            HATN_CHECK_RETURN(setMillisecond(millisecond))

            return OK;
        }

        uint64_t toNumber() const noexcept
        {
            uint64_t result=m_hour*10000000 + m_minute*100000 + m_second*1000 + m_millisecond;
            return result;
        }

        void reset()
        {
            m_hour=0;
            m_minute=0;
            m_second=0;
            m_millisecond=0;
        }

        bool isValid() const noexcept
        {
            return m_hour>0 || m_minute>0 || m_second>0 || m_millisecond>0;
        }

        bool isNull() const noexcept
        {
            return !isValid();
        }

        operator bool() const noexcept
        {
            return isValid();
        }

        template <typename T>
        Error setHour(T value) noexcept
        {
            if (value>=24)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_hour=static_cast<decltype(m_hour)>(value);
            return OK;
        }

        template <typename T>
        Error setMinute(T value) noexcept
        {
            if (value>=60)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_minute=static_cast<decltype(m_minute)>(value);
            return OK;
        }

        template <typename T>
        Error setSecond(T value) noexcept
        {
            if (value>=60)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_second=static_cast<decltype(m_second)>(value);
            return OK;
        }

        template <typename T>
        Error setMillisecond(T value) noexcept
        {
            if (value>=1000)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_millisecond=static_cast<decltype(m_millisecond)>(value);
            return OK;
        }

        static Result<Time> parse(const lib::string_view& str, FormatPrecision precision=FormatPrecision::Second, bool ampm=false);

        std::string toString(FormatPrecision precision=FormatPrecision::Second, bool ampm=false) const;

        static Time currentUtc();
        static Time currentLocal();

        template <typename HourT, typename MinuteT, typename SecondT, typename MillisecondT>
        static Error validate(HourT hour, MinuteT minute, SecondT second, MillisecondT ms=0) noexcept
        {
            if (hour>=24 || minute>=60 || second>=60 || ms>=1000)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            if (hour==0 && minute==0 && second==0 && ms==0)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            return OK;
        }

        bool operator==(const Time& other) const noexcept
        {
            return m_hour==other.m_hour&& m_minute==other.m_minute && m_second==other.m_second
                && m_millisecond==other.m_millisecond;
        }

        bool operator<(const Time& other) const noexcept
        {
            return toNumber()<other.toNumber();
        }

        bool operator<=(const Time& other) const noexcept
        {
            return toNumber()<=other.toNumber();
        }

        bool operator>(const Time& other) const noexcept
        {
            return toNumber()>other.toNumber();
        }

        bool operator>=(const Time& other) const noexcept
        {
            return toNumber()>=other.toNumber();
        }

    private:

        uint8_t m_hour;
        uint8_t m_minute;
        uint8_t m_second;
        uint16_t m_millisecond;

        Error validate() noexcept
        {
            auto ec=validate(m_hour,m_minute,m_second,m_millisecond);
            if (ec)
            {
                reset();
            }
            return ec;
        }

        friend class DateTime;
};

class HATN_COMMON_EXPORT DateTime
{
    public:

        DateTime():m_tz(0)
        {}

        template <typename TzT>
        DateTime(Date date, Time time, TzT tz=0)
            :m_date(std::move(date)),m_time(std::move(time)),m_tz(static_cast<decltype(m_tz)>(tz))
        {
            HATN_CHECK_THROW(validate())
        }

        const Date& date() const
        {
            return m_date;
        }

        Date& date()
        {
            return m_date;
        }

        const Time& time() const
        {
            return m_time;
        }

        Time& time()
        {
            return m_time;
        }

        int8_t tz() const noexcept
        {
            return m_tz;
        }

        template <typename T>
        Error setTz(T tz) noexcept
        {
            HATN_CHECK_RETURN(validateTz(tz))
            m_tz=static_cast<decltype(m_tz)>(tz);
            return OK;
        }

        std::string toIsoString(bool withMilliseconds=true) const;

        DateTime toUtc() const
        {
            return toTz(*this).takeValue();
        }

        DateTime toLocal() const
        {
            return toTz(*this,localTz()).takeValue();
        }

        uint64_t toEpochMs() const;

        uint32_t toEpoch() const;

        static Result<DateTime> parseIsoString(const lib::string_view& format);

        static DateTime currentUtc();

        static DateTime currentLocal();

        static uint64_t sinceEpochMs();

        static uint32_t sinceEpoch();

        static Result<DateTime> utcFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,0);
        }

        static Result<DateTime> localFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,localTz());
        }

        static Result<DateTime> utcFromEpoch(uint32_t seconds)
        {
            return fromEpoch(seconds,0);
        }

        static Result<DateTime> localFromEpoch(uint32_t seconds)
        {
            return fromEpoch(seconds,localTz());
        }

        static Result<DateTime> fromEpochMs(uint64_t seconds, int8_t tz=0);

        static Result<DateTime> fromEpoch(uint32_t seconds, int8_t tz=0);

        static int8_t localTz();

        static Result<DateTime> toTz(const DateTime& from, int8_t tz=0);

        template <typename T>
        static Result<DateTime> toTz(const DateTime& from, T tz=0)
        {
            return toTz(from,static_cast<decltype(m_tz)>(tz));
        }

        template <typename T>
        static Error validateTz(T tz)
        {
            if ((tz < (-12)) || tz>12)
            {
                return CommonError::INVALID_DATETIME_FORMAT;
            }
            return OK;
        }

        bool before(const DateTime& other) const
        {
            return sinceEpochMs()<other.sinceEpochMs();
        }

        bool after(const DateTime& other) const
        {
            return sinceEpochMs()>other.sinceEpochMs();
        }

        bool equal(const DateTime& other) const
        {
            return sinceEpochMs()==other.sinceEpochMs();
        }

        bool beforeOrEqual(const DateTime& other) const
        {
            return sinceEpochMs()<=other.sinceEpochMs();
        }

        bool afterOrEqual(const DateTime& other) const
        {
            return sinceEpochMs()>=other.sinceEpochMs();
        }

        bool operator ==(const DateTime& other) const noexcept
        {
            return m_tz==other.m_tz && m_date==other.m_date && m_time==other.m_time;
        }

        bool operator <(const DateTime& other) const
        {
            auto s=sinceEpochMs();
            auto o=other.sinceEpochMs();

            if (s<o)
            {
                return true;
            }
            if (s==o)
            {
                return m_tz<other.m_tz;
            }

            return false;
        }

        bool operator <=(const DateTime& other) const
        {
            auto s=sinceEpochMs();
            auto o=other.sinceEpochMs();

            if (s<o)
            {
                return true;
            }
            if (s==o)
            {
                return m_tz<=other.m_tz;
            }

            return false;
        }

        bool operator >(const DateTime& other) const
        {
            auto s=sinceEpochMs();
            auto o=other.sinceEpochMs();

            if (s>o)
            {
                return true;
            }
            if (s==o)
            {
                return m_tz>other.m_tz;
            }

            return false;
        }

        bool operator >=(const DateTime& other) const
        {
            auto s=sinceEpochMs();
            auto o=other.sinceEpochMs();

            if (s>o)
            {
                return true;
            }
            if (s==o)
            {
                return m_tz>=other.m_tz;
            }

            return false;
        }

        void addDays(int value)
        {
            m_date.addDays(value);
        }

        void addHours(int value);

        void addMinutes(int value);

        void addSeconds(int value);

        void addMilliseconds(int value);

    private:

        Error validate() noexcept
        {
            HATN_CHECK_RETURN(m_date.validate())
            HATN_CHECK_RETURN(m_time.validate())
            HATN_CHECK_RETURN(validateTz(m_tz))
            return OK;
        }

        Date m_date;
        Time m_time;
        int8_t m_tz;
};

/**
 * @brief Date range contains range the dates formatted as:
 * <type[0]><year[0:3]><range_pos[0:2]>.
 */
class HATN_COMMON_EXPORT DateRange
{
    public:

        enum Type : int
        {
            Year=0,
            HalfYear=1,
            Quarter=2,
            Month=3,
            Week=4,
            Day=5
        };

        DateRange(const DateTime& dt, Type type=Type::Month)
            :DateRange(dt.date(),type)
        {}

        static uint32_t dateToRange(const Date& dt, Type type=Type::Month);

        DateRange(const Date& dt, Type type=Type::Month)
            :m_value(dateToRange(dt,type))
        {}

        uint32_t value() const noexcept
        {
            return m_value;
        }

        Type type() const noexcept
        {
            return static_cast<Type>(m_value/(1000*10000));
        }

        uint32_t range() const noexcept
        {
            return m_value%1000;
        }

        uint16_t year() const noexcept
        {
            return (m_value/1000)%10000;
        }

        Date begin() const;

        Date end() const;

        DateTime beginDateTime() const
        {
            return DateTime{begin(),Time{0,0,1,0},0};
        }

        DateTime endDateTime() const
        {
            return DateTime{end(),Time{23,59,59,0},0};
        }

        bool contains(const Date& dt) const;

        bool contains(const DateTime& dt) const
        {
            return contains(dt.date());
        }

    private:

        uint32_t m_value;
};


HATN_COMMON_NAMESPACE_END
#endif // HATNDATETIME_H
