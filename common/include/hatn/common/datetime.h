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

#include <cmath>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/stdwrappers.h>

HATN_COMMON_NAMESPACE_BEGIN

class DateTime;

/**
 * @brief The Date class.
 */
class HATN_COMMON_EXPORT Date
{
    public:

        /**
         * @brief The Format enum.
         */
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

        /**
         * @brief Default ctor.
         */
        Date(): m_year(static_cast<decltype(m_year)>(0)),
                m_month(static_cast<decltype(m_month)>(0)),
                m_day(static_cast<decltype(m_day)>(0))
        {}

        /**
         * @brief Ctor with init.
         * @param year Year.
         * @param month Month.
         * @param day day.
         */
        template <typename YearT, typename MonthT, typename DayT>
        Date(YearT year, MonthT month, DayT day)
            : m_year(static_cast<decltype(m_year)>(year)),
              m_month(static_cast<decltype(m_month)>(month)),
              m_day(static_cast<decltype(m_day)>(day))
        {
            HATN_CHECK_THROW(validate())
        }

         /**
         * @brief Ctor from single number.
         * @param value Number in format (year*10000 + month*100 + day).
         */
        Date(uint32_t value)
        {
            HATN_CHECK_THROW(set(value));
        }

        /**
         * @brief Get year.
         * @return Year.
         */
        uint16_t year() const noexcept
        {
            return m_year;
        }

        /**
         * @brief Get month.
         * @return Month.
         */
        uint8_t month() const noexcept
        {
            return m_month;
        }

        /**
         * @brief Get day.
         * @return Day.
         */
        uint8_t day() const noexcept
        {
            return m_day;
        }

        /**
         * @brief Set date frm single number.
         * @param value Number in format (year*10000 + month*100 + day).
         * @return Validation result.
         */
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

        /**
         * @brief Get date as single number.
         * @return Number in format (year*10000 + month*100 + day).
         */
        uint32_t toNumber() const noexcept
        {
            uint32_t result=m_year*10000 + m_month*100 + m_day;
            return result;
        }

        /**
         * @brief setYear
         * @param value Year.
         * @return Validation result.
         */
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

        /**
         * @brief setMonth
         * @param value Month.
         * @return Validation result.
         */
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

        /**
         * @brief setDay
         * @param value Day.
         * @return Validation result.
         */
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

        /**
         * @brief Reset date to default invalid state.
         */
        void reset() noexcept
        {
            m_year=static_cast<decltype(m_year)>(0);
            m_month=static_cast<decltype(m_month)>(0);
            m_day=static_cast<decltype(m_day)>(0);
        }

        /**
         * @brief Check if object is valid.
         * @return Operation result.
         */
        bool isValid() const noexcept
        {
            return m_year>=1970 || m_month>=1 || m_day>=1;
        }

        /**
         * @brief Check if object is null.
         * @return Operation result.
         */
        bool isNull() const noexcept
        {
            return !isValid();
        }

        /**
         * @brief Parse date from string.
         * @param str String.
         * @param format Format.
         * @param baseShortYear Base year in case of short year format.
         * @return Operation result.
         */
        static Result<Date> parse(
            const lib::string_view& str,
            Format format=Format::Iso,
            uint16_t baseShortYear=2000
        );

        /**
         * @brief Format date to string.
         * @param format Format.
         * @return Formatted date.
         */
        std::string toString(Format format=Format::Iso) const;

        /**
         * @brief Get current UTC date.
         * @return Current UTC date.
         */
        static Date currentUtc();

        /**
         * @brief Get current local date.
         * @return Current local date.
         */
        static Date currentLocal();

        bool operator==(const Date& other) const noexcept
        {
            return m_year==other.m_year && m_month==other.m_month && m_day==other.m_day;
        }

        bool operator!=(const Date& other) const noexcept
        {
            return !(*this==other);
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

        bool isLeapYear() const
        {
            return isLeapYear(m_year);
        }

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

/**
 * @brief The Time class.
 */
class HATN_COMMON_EXPORT Time
{
    public:

        /**
         * @brief The FormatPrecision enum.
         */
        enum class FormatPrecision : int
        {
            Minute,
            Second,
            Millisecond,
            Number
        };

        /**
         * @brief Default ctor.
         */
        Time(): m_hour(static_cast<decltype(m_hour)>(0)),
                m_minute(static_cast<decltype(m_minute)>(0)),
                m_second(static_cast<decltype(m_second)>(0)),
                m_millisecond(static_cast<decltype(m_millisecond)>(0))
        {}

        /**
         * @brief Ctor with init.
         * @param hour Hour.
         * @param minute Minute.
         * @param second Second.
         * @param ms Millisecond.
         */
        template <typename HourT, typename MinuteT, typename SecondT, typename MillisecondT>
        Time(HourT hour, MinuteT minute, SecondT second, MillisecondT ms)
            : m_hour(static_cast<decltype(m_hour)>(hour)),
            m_minute(static_cast<decltype(m_minute)>(minute)),
            m_second(static_cast<decltype(m_second)>(second)),
            m_millisecond(static_cast<decltype(m_millisecond)>(ms))
        {
            HATN_CHECK_THROW(validate())
        }

        /**
         * @brief Ctor with init.
         * @param hour Hour.
         * @param minute Minute.
         * @param second Second.
         */
        template <typename HourT, typename MinuteT, typename SecondT>
        Time(HourT hour, MinuteT minute, SecondT second)
        : Time(hour,minute,second,0)
        {}

        /**
         * @brief Ctor from single number.
         * @param value Number in format (hour*10000000 + minute*100000 + second*1000 + millisecond).
         */
        Time(uint64_t value)
        {
            auto ec=set(value);
            if (ec)
            {
                throw ErrorException{ec};
            }
        }

        /**
         * @brief Get hour.
         * @return Hour.
         */
        uint8_t hour() const noexcept
        {
            return m_hour;
        }

        /**
         * @brief Get minute.
         * @return Minute.
         */
        uint8_t minute() const noexcept
        {
            return m_minute;
        }

        /**
         * @brief Get second.
         * @return Second.
         */
        uint8_t second() const noexcept
        {
            return m_second;
        }

        /**
         * @brief Get millisecond.
         * @return Millisecond.
         */
        uint16_t millisecond() const noexcept
        {
            return m_millisecond;
        }

        /**
         * @brief Set time from single number.
         * @param value Number in format (hour*10000000 + minute*100000 + second*1000 + millisecond).
         * @return Validation result.
         */
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

        /**
         * @brief Get time as single number.
         * @return Number in format (hour*10000000 + minute*100000 + second*1000 + millisecond).
         */
        uint64_t toNumber() const noexcept
        {
            uint64_t result=m_hour*10000000 + m_minute*100000 + m_second*1000 + m_millisecond;
            return result;
        }

        /**
         * @brief Reset time to default invalid state.
         */
        void reset()
        {
            m_hour=0;
            m_minute=0;
            m_second=0;
            m_millisecond=0;
        }

        /**
         * @brief Check if object is valid.
         * @return Operation result.
         */
        bool isValid() const noexcept
        {
            return m_hour>0 || m_minute>0 || m_second>0 || m_millisecond>0;
        }

        /**
         * @brief Check if object is null.
         * @return Operation result.
         */
        bool isNull() const noexcept
        {
            return !isValid();
        }

        operator bool() const noexcept
        {
            return isValid();
        }

        /**
         * @brief setHour.
         * @param value Hour.
         * @return Validation result.
         */
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

        /**
         * @brief setMinute.
         * @param value Minute.
         * @return Validation result.
         */
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

        /**
         * @brief setSecond.
         * @param value Second.
         * @return Validation result.
         */
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

        /**
         * @brief setMillisecond.
         * @param value Millisecond.
         * @return Validation result.
         */
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

        /**
         * @brief Parse time from string.
         * @param str Source string.
         * @param precision Format precision.
         * @param ampm Truu if AM/PM mode is used.
         * @return Operation result.
         */
        static Result<Time> parse(
            const lib::string_view& str,
            FormatPrecision precision=FormatPrecision::Second,
            bool ampm=false
        );

        /**
         * @brief Format time to string.
         * @param precision Format precision.
         * @param ampm Truu if AM/PM mode must be used.
         * @return Formatted time.
         */
        std::string toString(
            FormatPrecision precision=FormatPrecision::Second,
            bool ampm=false
        ) const;

        /**
         * @brief get current UTC time.
         * @return Current UTC time.
         */
        static Time currentUtc();

        /**
         * @brief get current local time.
         * @return Current local time.
         */
        static Time currentLocal();

        /**
         * @brief Validate time fields.
         * @param hour Hour.
         * @param minute Minute.
         * @param second Second.
         * @param ms Millisecond.
         * @return Validation result.
         */
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

        /**
         * @brief addHours.
         * @param value Hours.
         */
        void addHours(int value)
        {
            auto hour=(m_hour+value)%24;
            if (hour<0)
            {
                hour+=24;
            }
            m_hour=hour;
        }

        /**
         * @brief addMinutes.
         * @param value Minutes.
         */
        void addMinutes(int value)
        {
            if (abs(value)>=60)
            {
                addHours(value/60);
            }
            auto minute=m_minute+value%60;
            if (minute<0)
            {
                addHours(-1);
                minute+=60;
            } else if (minute>=60)
            {
                addHours(1);
                minute-=60;
            }
            m_minute=minute;
        }

        /**
         * @brief addSeconds.
         * @param value Seconds.
         */
        void addSeconds(int value)
        {
            if (abs(value)>=60)
            {
                addMinutes(value/60);
            }
            auto second=m_second+value%60;
            if (second<0)
            {
                addMinutes(-1);
                second+=60;
            } else if (second>=60)
            {
                addMinutes(1);
                second-=60;
            }
            m_second=second;
        }

        /**
         * @brief addMilliseconds.
         * @param value Milliseconds.
         */
        void addMilliseconds(int value)
        {
            auto s=value/1000;
            if (abs(s)>0)
            {
                addSeconds(s);
            }
            auto millisecond=m_millisecond+value%1000;
            if (millisecond<0)
            {
                addSeconds(-1);
                millisecond+=1000;
            } else if (millisecond>=1000)
            {
                addSeconds(1);
                millisecond-=1000;
            }
            m_millisecond=millisecond;
        }

        bool operator==(const Time& other) const noexcept
        {
            return m_hour==other.m_hour&& m_minute==other.m_minute && m_second==other.m_second
                && m_millisecond==other.m_millisecond;
        }

        bool operator!=(const Time& other) const noexcept
        {
            return !(*this==other);
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

/**
 * @brief The DateTime class.
 */
class HATN_COMMON_EXPORT DateTime
{
    public:

        /**
         * @brief The Diff class returned by diff() method.
         */
        struct Diff
        {
            int32_t hours;
            int32_t minutes;
            int32_t seconds;
        };

        //! Default ctor.
        DateTime():m_tz(0)
        {}

        /**
         * @brief Ctor with init.
         * @param date Date.
         * @param time Time.
         * @param tz Timezone.
         */
        template <typename TzT>
        DateTime(Date date, Time time, TzT tz)
            :m_date(std::move(date)),m_time(std::move(time)),m_tz(static_cast<decltype(m_tz)>(tz))
        {
            HATN_CHECK_THROW(validate())
        }

        /**
         * @brief Ctor with init.
         * @param date Date.
         * @param time Time.
         */
        DateTime(Date date, Time time)
            : DateTime(std::move(date),std::move(time),0)
        {}

        /**
         * @brief Get date.
         * @return Const date reference.
         */
        const Date& date() const
        {
            return m_date;
        }

        /**
         * @brief Get date.
         * @return Date reference.
         */
        Date& date()
        {
            return m_date;
        }

        /**
         * @brief Get time.
         * @return Const time reference.
         */
        const Time& time() const
        {
            return m_time;
        }

        /**
         * @brief Get time.
         * @return Time reference.
         */
        Time& time()
        {
            return m_time;
        }

        /**
         * @brief Get timezone.
         * @return Timezone.
         */
        int8_t tz() const noexcept
        {
            return m_tz;
        }

        /**
         * @brief Check if datetime is valid.
         * @return True if valid.
         */
        bool isValid() const noexcept
        {
            return m_date.isValid() && m_time.isValid();
        }

        /**
         * @brief Check if datetime is null.
         * @return True if not valid.
         */
        bool isNull() const noexcept
        {
            return !isValid();
        }

        /**
         * @brief Set time zone.
         * @param tz New timezone.
         * @return OK if tz is valid.
         *
         * This method changes timezone as is without updating time and date.
         * To convert datetime to new timezone uset toTz() method.
         */
        template <typename T>
        Error setTz(T tz) noexcept
        {
            HATN_CHECK_RETURN(validateTz(tz))
            m_tz=static_cast<decltype(m_tz)>(tz);
            return OK;
        }

        /**
         * @brief Format datetime as ISO string.
         * @param withMilliseconds Show milliseconds.
         * @return Formatted string.
         */
        std::string toIsoString(bool withMilliseconds=true) const;

        /**
         * @brief Convert datetime to UTC.
         * @return UTC datetime.
         */
        DateTime toUtc() const
        {
            return toTz(*this).takeValue();
        }

        /**
         * @brief Convert datetime to local.
         * @return UTC datetime.
         */
        DateTime toLocal() const
        {
            return toTz(*this,localTz()).takeValue();
        }

        /**
         * @brief Get milliseconds since epoch.
         * @return Milliseconds since epoch.
         */
        uint64_t toEpochMs() const;

        /**
         * @brief Get seconds since epoch.
         * @return Seconds since epoch.
         */
        uint32_t toEpoch() const;

        /**
         * @brief Parse datetime in ISO format.
         * @param format String in ISO format.
         * @return Parsing result.
         *
         * Supported formats:
         * 2023-10-05T21:35:47.123Z for UTC with milliseconds
         * 20231005T21:35:47.123Z for UTC with milliseconds
         * 2023-10-05T21:35:47Z for UTC
         * 20231005T21:35:47Z for UTC
         * 2023-10-05T21:35:47.123+03:00 with timezone with milliseconds
         * 20231005T21:35:47.123-03:00 for with timezone with milliseconds
         * 2023-10-05T21:35:47+03:00 with timezone
         * 20231005T21:35:47-03:00 for with timezone
         */
        static Result<DateTime> parseIsoString(const lib::string_view& str);

        /**
         * @brief Get current UTC datetime.
         * @return Current UTC datetime.
         */
        static DateTime currentUtc();

        /**
         * @brief Get current local datetime.
         * @return Current local datetime.
         */
        static DateTime currentLocal();

        void toCurrentUtc();

        void toCurrentLocal();

        /**
         * @brief Construct datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @param tz Timezone.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> fromEpochMs(uint64_t milliseconds, int8_t tz=0);

        /**
         * @brief Construct datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @param tz Timezone.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> fromEpoch(uint32_t seconds, int8_t tz=0);

        /**
         * @brief Construct UTC datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> utcFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,0);
        }

        /**
         * @brief Construct local datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> localFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,localTz());
        }

        /**
         * @brief Construct UTC datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> utcFromEpoch(uint32_t seconds)
        {
            return fromEpoch(seconds,0);
        }

        /**
         * @brief Construct local datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or error.
         */
        static Result<DateTime> localFromEpoch(uint32_t seconds)
        {
            return fromEpoch(seconds,localTz());
        }

        /**
         * @brief Get local timezone.
         * @return Local timezone.
         */
        static int8_t localTz();

        /**
         * @brief Convert datetime to datetime with given timezone.
         * @param from Original datetime.
         * @param tz Timezone.
         * @return Converted datetime or error.
         */
        static Result<DateTime> toTz(const DateTime& from, int8_t tz=0);

        /**
         * @brief Convert datetime to datetime with given timezone.
         * @param from Original datetime.
         * @param tz Timezone.
         * @return Converted datetime or error.
         */
        template <typename T>
        static Result<DateTime> toTz(const DateTime& from, T tz=0)
        {
            return toTz(from,static_cast<decltype(m_tz)>(tz));
        }

        /**
         * @brief Validate timezone.
         * @param tz Timezone.
         * @return Validation result.
         */
        template <typename T>
        static Error validateTz(T tz)
        {
            if (abs(tz)>12)
            {
                return CommonError::INVALID_DATETIME_FORMAT;
            }
            return OK;
        }

        /**
         * @brief Get current milliseconds since epoch.
         * @return Milliseconds since epoch till now.
         */
        static uint64_t millisecondsSinceEpoch();

        /**
         * @brief Get current seconds since epoch.
         * @return Seconds since epoch till now.
         */
        static uint32_t secondsSinceEpoch();

        /**
         * @brief Check if datetime is before other regardless of timezone.
         * @param other Other datetime.
         * @return True if this is before other.
         */
        bool before(const DateTime& other) const
        {
            return toEpochMs()<other.toEpochMs();
        }

        /**
         * @brief Check if datetime is after other regardless of timezone.
         * @param other Other datetime.
         * @return True if this is after other.
         */
        bool after(const DateTime& other) const
        {
            return toEpochMs()>other.toEpochMs();
        }

        /**
         * @brief Check if datetimes are equal regardless of timezone.
         * @param other Other datetime.
         * @return True if datetimes are equal.
         */
        bool equal(const DateTime& other) const
        {
            return toEpochMs()==other.toEpochMs();
        }

        /**
         * @brief Check if datetime is before or equal to other regardless of timezone.
         * @param other Other datetime.
         * @return True if this is before or equal to other.
         */
        bool beforeOrEqual(const DateTime& other) const
        {
            return toEpochMs()<=other.toEpochMs();
        }

        /**
         * @brief Check if datetime is after or equal to other regardless of timezone.
         * @param other Other datetime.
         * @return True if this is after or equal to other.
         */
        bool afterOrEqual(const DateTime& other) const
        {
            return toEpochMs()>=other.toEpochMs();
        }

        /**
         * @brief Equality operator, datetimes are equal if all their members are equal including timezones.
         * @param other Other datetime.
         * @return True if datetimes are equal.
         */
        bool operator ==(const DateTime& other) const noexcept
        {
            return m_tz==other.m_tz && m_date==other.m_date && m_time==other.m_time;
        }

        /**
         * @brief operator != negates operator ==.
         * @param other Other datetime.
         * @return Operation result.
         */
        bool operator !=(const DateTime& other) const noexcept
        {
            return !(*this==other);
        }

        /**
         * @brief operator < compares datetimes since epoch and timezones.
         * @param other Other datetime.
         * @return Operation result.
         */
        bool operator <(const DateTime& other) const
        {
            auto s=toEpochMs();
            auto o=other.toEpochMs();

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

        /**
         * @brief operator <= compares datetimes since epoch and timezones.
         * @param other Other datetime.
         * @return Operation result.
         */
        bool operator <=(const DateTime& other) const
        {
            auto s=toEpochMs();
            auto o=other.toEpochMs();

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

        /**
         * @brief operator > compares datetimes since epoch and timezones.
         * @param other Other datetime.
         * @return Operation result.
         */
        bool operator >(const DateTime& other) const
        {
            auto s=toEpochMs();
            auto o=other.toEpochMs();

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

        /**
         * @brief operator >= compares datetimes since epoch and timezones.
         * @param other Other datetime.
         * @return Operation result.
         */
        bool operator >=(const DateTime& other) const
        {
            auto s=toEpochMs();
            auto o=other.toEpochMs();

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

        /**
         * @brief Add days.
         * @param value Days to add.
         */
        void addDays(int value)
        {
            m_date.addDays(value);
        }

        /**
         * @brief Add hours.
         * @param value Hours to add.
         */
        void addHours(int value);

        /**
         * @brief Add minutes.
         * @param value Minutes to add.
         */
        void addMinutes(int value);

        /**
         * @brief Add seconds.
         * @param value Seconds to add.
         */
        void addSeconds(int value);

        /**
         * @brief Add milliseconds.
         * @param value Milliseconds to add.
         */
        void addMilliseconds(int value);

        /**
         * @brief Calculate difference with other datetime in milliseconds.
         * @param other Other datetime.
         * @return Difference in milliseconds.
         */
        int64_t diffMilliseconds(const DateTime& other) const
        {
            return static_cast<int64_t>(toEpochMs())-static_cast<int64_t>(other.toEpochMs());
        }

        /**
         * @brief Calculate difference with other datetime in seconds.
         * @param other Other datetime.
         * @return Difference in seconds.
         */
        int32_t diffSeconds(const DateTime& other) const
        {
            return toEpoch()-other.toEpoch();
        }

        /**
         * @brief Calculate difference with other datetime.
         * @param other Other datetime.
         * @return Difference.
         */
        Diff diff(const DateTime& other) const
        {
            auto d=diffSeconds(other);
            Diff r;
            r.hours=d/3600;
            r.minutes=(d-r.hours*3600)/60;
            r.seconds=d%60;
            return r;
        }

        /**
         * @brief Convert datetime to single number.
         * @return Operation result.
         */
        uint64_t toNumber() const
        {
            if (isNull())
            {
                return 0;
            }

            if (m_tz==0)
            {
                return toEpochMs();
            }

            auto ep=toEpochMs();
            auto tz=static_cast<uint8_t>(m_tz);
            return ep + (static_cast<uint64_t>(tz)<<48);
        }

        /**
         * @brief Construct datetime from single number.
         * @param num Number.
         * @return Operation result.
         */
        static Result<DateTime> fromNumber(uint64_t num)
        {
            if (num==0)
            {
                return DateTime{};
            }

            auto tz=num>>48;
            if (tz==0)
            {
                return utcFromEpochMs(num);
            }

            auto epochMs=num&0xFFFFFFFFFFFF;
            return fromEpochMs(epochMs,static_cast<int8_t>(tz));
        }

        /**
         * @brief Set datetime from single number.
         * @param num Number.
         * @return Operation status.
         */
        Error setNumber(uint64_t num)
        {
            auto r=fromNumber(num);
            if (r)
            {
                return r.takeError();
            }
            *this=r.takeValue();
            return OK;
        }

        /**
         * @brief Reset date time.
         */
        void reset() noexcept
        {
            m_date.reset();
            m_time.reset();
            m_tz=0;
        }

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
 * @brief Date range defines range the date can belong to formatted as:
 * <type[0]><year[0:3]><range[0:2]>.
 */
class HATN_COMMON_EXPORT DateRange
{
    public:

        /**
         * @brief Type of the range.
         */
        enum Type : int
        {
            Year=0,
            HalfYear=1,
            Quarter=2,
            Month=3,
            Week=4,
            Day=5
        };

        /**
         * @brief Ctor from number.
         * @param value Number.
         */
        DateRange(uint32_t value=0) : m_value(value)
        {}

        /**
         * @brief Ctor from DateTime.
         * @param dt DateTime
         * @param type Range type.
         */
        DateRange(const DateTime& dt, Type type=Type::Month)
            :DateRange(dt.date(),type)
        {}

        /**
         * @brief Ctor from Date.
         * @param dt Date
         * @param type Range type.
         */
        DateRange(const Date& dt, Type type=Type::Month)
            :m_value(dateToRangeNumber(dt,type))
        {}

        /**
         * @brief Check if date range is valid.
         * @return Operation restult.
         */
        bool isValid() const noexcept
        {
            return m_value!=0;
        }

        /**
         * @brief Construct range from date.
         * @param dt Date.
         * @param type Range type.
         * @return Constructed date range.
         */
        static DateRange dateToRange(const Date& dt, Type type=Type::Month);

        /**
         * @brief Calculate range number from date.
         * @param dt Date.
         * @param type Range type.
         * @return Number of date range.
         */
        static uint32_t dateToRangeNumber(const Date& dt, Type type=Type::Month);

        /**
         * @brief Get date rage number.
         * @return Date range number.
         */
        uint32_t value() const noexcept
        {
            return m_value;
        }

        /**
         * @brief Get date range type.
         * @return Date range type.
         */
        Type type() const noexcept
        {
            return static_cast<Type>(m_value/(1000*10000));
        }

        /**
         * @brief Get range.
         * @return Range.
         */
        uint32_t range() const noexcept
        {
            return m_value%1000;
        }

        /**
         * @brief Get year.
         * @return Year.
         */
        uint16_t year() const noexcept
        {
            return (m_value/1000)%10000;
        }

        /**
         * @brief Figure out beginning date of date range.
         * @return Beginning date of the date range.
         */
        Date begin() const;

        /**
         * @brief Figure out end date of date range.
         * @return End date of the date range.
         */
        Date end() const;

        /**
         * @brief Figure out beginning date time of date range with seconds precision.
         * @return Beginning date time of the date range.
         */
        DateTime beginDateTime() const
        {
            return DateTime{begin(),Time{0,0,1,0},0};
        }

        /**
         * @brief Figure out end date time of date range with seconds precision.
         * @return End date time of the date range.
         */
        DateTime endDateTime() const
        {
            return DateTime{end(),Time{23,59,59,0},0};
        }

        /**
         * @brief Check if date range contains a date.
         * @param dt Date.
         * @return Operation result.
         */
        bool contains(const Date& dt) const;

        /**
         * @brief Check if date range contains a date time.
         * @param dt Date time.
         * @return Operation result.
         */
        bool contains(const DateTime& dt) const
        {
            return contains(dt.date());
        }

    private:

        uint32_t m_value;
};

HATN_COMMON_NAMESPACE_END
#endif // HATNDATETIME_H
