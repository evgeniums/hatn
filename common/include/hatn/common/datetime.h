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

class HATN_COMMON_EXPORT Date
{
    public:

        Date():Date(0,0,0)
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
            m_year=value/10000;
            m_month=(value-m_year*10000)/100;
            m_day=value-m_year*10000-m_month*100;
            HATN_CHECK_RETURN(validate())
            return OK;
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
            setYear(0);
            setMonth(0);
            setDay(0);
        }

        bool isValid() const noexcept
        {
            return m_year>=1970 || m_month>=1 || m_day>=1;
        }

        bool isNull() const noexcept
        {
            return !isValid();
        }

        static Result<Date> parse(const lib::string_view& str);

        std::string toString(const lib::string_view& format=lib::string_view{}) const;

        static Date currentUtc();

    private:

        uint16_t m_year;
        uint8_t m_month;
        uint8_t m_day;

        Error validate() noexcept
        {
            //! @todo check number of days per month
            if (m_month>12 || m_day>31 || m_month==0 || m_day==0 || m_year==0)
            {
                reset();
                return CommonError::INVALID_DATE_FORMAT;
            }
            return OK;
        }
};

class HATN_COMMON_EXPORT Time
{
    public:

        Time():Time(0,0,0,0)
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
            m_hour=value/10000000;
            m_minute=(value-m_hour*10000000)/100000;
            m_second=(value-m_hour*10000000-m_minute*100000)/1000;
            m_millisecond=value-m_hour*10000000-m_minute*100000-m_second*1000;
            HATN_CHECK_RETURN(validate())
            return OK;
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
        }

        template <typename T>
        Error setMinute(T value) noexcept
        {
            if (value>=60)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_minute=static_cast<decltype(m_minute)>(value);
        }

        template <typename T>
        Error setSecond(T value) noexcept
        {
            if (value>=60)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_second=static_cast<decltype(m_second)>(value);
        }

        template <typename T>
        Error setMillisecond(T value) noexcept
        {
            if (value>=60)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            m_millisecond=static_cast<decltype(m_millisecond)>(value);
        }

        static Result<Time> parse(const lib::string_view& format);

        std::string toString(const lib::string_view& format=lib::string_view{}) const;

        static Time currentUtc();

    private:

        uint8_t m_hour;
        uint8_t m_minute;
        uint8_t m_second;
        uint16_t m_millisecond;

        Error validate() noexcept
        {
            if (m_hour>=24 || m_minute>=60 || m_second>=60 || m_millisecond>=1000)
            {
                reset();
                return CommonError::INVALID_TIME_FORMAT;
            }
            return OK;
        }
};

//! @todo Implement timezone
class HATN_COMMON_EXPORT TimeZone
{
    public:

        TimeZone():m_name("UTC"),m_offset(0)
        {}

        int8_t offset() const noexcept
        {
            return m_offset;
        }

        const std::string& name() const noexcept
        {
            return m_name;
        }

        template <typename T>
        Error setOffset(T value)
        {
            return setOffset(static_cast<int8_t>(value));
        }

        Error setOffset(int8_t value);

        Error setName(std::string name);

    private:

        std::string m_name;
        int8_t m_offset;
};

class HATN_COMMON_EXPORT DateTimeUtc
{
    public:

        DateTimeUtc();

        DateTimeUtc(Date date, Time time):m_date(std::move(date)),m_time(std::move(time))
        {}

        virtual ~DateTimeUtc();
        DateTimeUtc(const DateTimeUtc&)=default;
        DateTimeUtc(DateTimeUtc&&)=default;
        DateTimeUtc& operator=(const DateTimeUtc&)=default;
        DateTimeUtc& operator=(DateTimeUtc&&)=default;

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

        virtual std::string toString(const lib::string_view& format=lib::string_view{}) const;

        static Result<DateTimeUtc> parse(const lib::string_view& str);

        static DateTimeUtc currentUtc();

        static uint64_t msSinceEpoch();

        static Result<DateTimeUtc> fromMsSinceEpoch(uint64_t value);

    private:

        Date m_date;
        Time m_time;
};

//! @todo Implement DateTime
class HATN_COMMON_EXPORT DateTime : public DateTimeUtc
{
    public:

        DateTime();

        DateTime(Date date, Time time, TimeZone tz=TimeZone{}):
            DateTimeUtc(std::move(date),std::move(time)),
            m_tz(std::move(tz))
        {}

        DateTime(DateTimeUtc dt, TimeZone tz=TimeZone{}):
            DateTimeUtc(std::move(dt)),
            m_tz(std::move(tz))
        {}

        virtual ~DateTime();
        DateTime(const DateTime&)=default;
        DateTime(DateTime&&)=default;
        DateTime& operator=(const DateTime&)=default;
        DateTime& operator=(DateTime&&)=default;

        const TimeZone& tz() const noexcept
        {
            return m_tz;
        }

        TimeZone& tz() noexcept
        {
            return m_tz;
        }

        static DateTime currentUtc();

        static DateTime currentLocal();

        virtual std::string toString(const lib::string_view& format=lib::string_view{}) const override;

        static Result<DateTime> parse(const lib::string_view& str);

        static DateTime toTz(const DateTime& from, TimeZone tz=TimeZone{});

        static DateTime toTz(const DateTimeUtc& from, TimeZone tz=TimeZone{});

        DateTimeUtc toUtc() const
        {
            auto utc=toTz(*this);
            return DateTimeUtc{static_cast<const DateTimeUtc&>(utc)};
        }

    private:

        TimeZone m_tz;
};

class DateRange
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
            :DateRange(dt.toUtc(),type)
        {}

        DateRange(const DateTimeUtc& dt, Type type=Type::Month)
            :DateRange(dt.date(),type)
        {}

        DateRange(const Date& dt, Type type=Type::Month)
            :m_value(0)
        {
            //! @todo implement
        }

        uint32_t value() const noexcept
        {
            return m_value;
        }

        Type type() const noexcept
        {
            //! @todo implement
            return Month;
        }

        Date begin() const noexcept
        {
            //! @todo implement
            return Date{};
        }

        Date end() const noexcept
        {
            //! @todo implement
            return Date{};
        }

        DateTime beginDateTime() const noexcept
        {
            //! @todo implement
            return DateTime{};
        }

        DateTime endDateTime() const noexcept
        {
            //! @todo implement
            return DateTime{};
        }

    private:

        uint32_t m_value;
};


HATN_COMMON_NAMESPACE_END
#endif // HATNDATETIME_H
