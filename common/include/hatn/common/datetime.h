/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/month.h
  *
  *      Month object.
  *
  */

/****************************************************************************/

#ifndef HATNMONTH_H
#define HATNMONTH_H

#include <chrono>
#include "boost/date_time/local_time/local_time.hpp"

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/stdwrappers.h>

HATN_COMMON_NAMESPACE_BEGIN

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
            :m_value(0)
        {
            //! @todo implement
        }

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

class HATN_COMMON_EXPORT Date
{
    public:

        template <typename YearT, typename MonthT, typename DayT>
        Date(YearT year, MonthT month, DayT day) noexcept
            : m_year(static_cast<decltype(m_year)>(year)),
              m_month(static_cast<decltype(m_month)>(month)),
              m_day(static_cast<decltype(m_day)>(day))
        {}

        Date(uint32_t value) noexcept
        {
            set(value);
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

        void set(uint32_t value) noexcept
        {
            m_year=value/10000;
            m_month=(value-m_year*10000)/100;
            m_day=value-m_year*10000-m_month*100;
        }

        template <typename T>
        void setYear(T value) noexcept
        {
            m_year=static_cast<decltype(m_year)>(value);
        }

        template <typename T>
        void setMonth(T value) noexcept
        {
            m_month=static_cast<decltype(m_month)>(value);
        }

        template <typename T>
        void setDay(T value) noexcept
        {
            m_day=static_cast<decltype(m_day)>(value);
        }

        static Result<Date> parse(const lib::string_view& str)
        {
            //! @todo implement
            return Error{CommonError::NOT_IMPLEMENTED};
        }

        std::string toString(const lib::string_view& format=lib::string_view{}) const
        {
            //! @todo implement
            return std::string{};
        }

    private:

        uint16_t m_year;
        uint8_t m_month;
        uint8_t m_day;
};

class HATN_COMMON_EXPORT Time
{
    public:

        constexpr static const char* UTC_TZ="UTC";

        Time():Time(0,0,0,0,0)
        {}

        template <typename HourT, typename MinuteT, typename SecondT, typename MillisecondT, typename OffsetT>
        Time(HourT hour, MinuteT minute, SecondT second, MillisecondT ms=0, OffsetT tzOffset=0) noexcept
            : m_hour(static_cast<decltype(m_hour)>(hour)),
            m_minute(static_cast<decltype(m_minute)>(minute)),
            m_second(static_cast<decltype(m_second)>(second)),
            m_millisecond(static_cast<decltype(m_millisecond)>(ms)),
            m_tzOffset(static_cast<int8_t>(tzOffset))
        {
            HATN_CHECK_THROW(validate())
            if (tzOffset==0)
            {
                m_tz=UTC_TZ;
            }
            else
            {
                //! @todo construct TZ relative to UTC
            }
        }

        template <typename HourT, typename MinuteT, typename SecondT, typename MillisecondT>
        Time(HourT hour, MinuteT minute, SecondT second, MillisecondT ms, std::string& tz) noexcept
            : m_hour(static_cast<decltype(m_hour)>(hour)),
            m_minute(static_cast<decltype(m_minute)>(minute)),
            m_second(static_cast<decltype(m_second)>(second)),
            m_millisecond(static_cast<decltype(m_millisecond)>(ms)),
            m_tz(std::move(tz)),
            m_tzOffset(0)
        {
            HATN_CHECK_THROW(validate())
            //! @todo extract tz offset from tz
        }

        Time(uint64_t value=0) noexcept
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
            if (validate())
            {
                reset();
            }
            return OK;
        }

        void reset()
        {
            m_hour=0;
            m_minute=0;
            m_second=0;
            m_millisecond=0;
            m_tzOffset=0;
            m_tz=UTC_TZ;
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

        int8_t tzOffset() const noexcept
        {
            //! @todo implement
            return 0;
        }

        std::string tzName() const
        {
            //! @todo implement
            return 0;
        }

        template <typename T>
        void setTz(int8_t tzOffset)
        {
            m_tzOffset=static_cast<decltype(m_tzOffset)>(value);
            //! @todo implement
        }

        void setTz(std::string tzName)
        {
            //! @todo implement
        }

        static Result<Time> parse(const lib::string_view& format)
        {
            //! @todo implement
            return Error{CommonError::NOT_IMPLEMENTED};
        }

        std::string toString(const lib::string_view& format=lib::string_view{}) const
        {
            //! @todo implement
            return std::string{};
        }

        static Time currentLocal()
        {
            //! @todo implement
            return Time{};
        }

        static Time currentUtc()
        {
            //! @todo implement
            return Time{};
        }

    private:

        uint8_t m_hour;
        uint8_t m_minute;
        uint8_t m_second;
        uint16_t m_millisecond;
        std::string m_tz;
        int8_t m_tzOffset;

        Error validate() const noexcept
        {
            if (m_hour>=24 || m_minute>=60 || m_second>=60 || m_millisecond>=1000)
            {
                return CommonError::INVALID_TIME_FORMAT;
            }
            return OK;
        }
};

class HATN_COMMON_EXPORT DateTime
{
    public:

        static uint64_t msSinceEpoch()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        std::string toString(const lib::string_view& format=lib::string_view{}) const
        {
            //! @todo implement
            return std::string{};
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

        static Result<DateTime> parse(const lib::string_view& str)
        {
            //! @todo implement
            return Error{CommonError::NOT_IMPLEMENTED};
        }

        static DateTime currentLocal()
        {
            //! @todo implement
            return DateTime{};
        }

        static DateTime currentUtc()
        {
            //! @todo implement
            return DateTime{};
        }

        static Result<DateTime> fromMsSinceEpoch(uint64_t value)
        {
            //! @todo implement
            return Error{CommonError::NOT_IMPLEMENTED};
        }

    private:

        Date m_date;
        Time m_time;
};

HATN_COMMON_NAMESPACE_END
#endif // HATNMONTH_H
