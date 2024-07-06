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
#include <hatn/common/date.h>
#include <hatn/common/time.h>

HATN_COMMON_NAMESPACE_BEGIN

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
        DateTime():m_tz(m_defaultTz)
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
         * @brief Serialize datetime to buffer as ISO string.
         * @param buf Target buffer.
         * @param withMilliseconds Show milliseconds.
         */
        template <typename BufT>
        void serialize(BufT &buf, bool withMilliseconds=true) const;

        /**
         * @brief Format datetime as ISO string.
         * @param withMilliseconds Show milliseconds.
         * @return Formatted string.
         */
        std::string toIsoString(bool withMilliseconds=true) const
        {
            FmtAllocatedBufferChar buf;
            serialize(buf,withMilliseconds);
            return fmtBufToString(buf);
        }

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

        /**
         * @brief Get current datetime for time zone.
         * @return Current datetime in specified time zone.
         */
        static DateTime current(int8_t tz);

        /**
         * @brief Get current datetime.
         * @return Current datetime in default time zone.
         */
        static DateTime current();

        static void setDefaultTz(int8_t tz);

        static int8_t defaultTz() noexcept;

        void loadCurrentUtc();

        void loadCurrentLocal();

        void loadCurrent(int8_t tz);

        void loadCurrent();

        /**
         * @brief Construct datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @param tz Timezone.
         * @return Constructed datetime.
         */
        static DateTime fromEpochMs(uint64_t milliseconds, int8_t tz=0);

        /**
         * @brief Construct datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @param tz Timezone.
         * @return Constructed datetime or error.
         */
        static DateTime fromEpoch(uint32_t seconds, int8_t tz=0);

        /**
         * @brief Construct UTC datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or error.
         */
        static DateTime utcFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,0);
        }

        /**
         * @brief Construct local datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or error.
         */
        static DateTime localFromEpochMs(uint64_t milliseconds)
        {
            return fromEpochMs(milliseconds,localTz());
        }

        /**
         * @brief Construct UTC datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or error.
         */
        static DateTime utcFromEpoch(uint32_t seconds)
        {
            return fromEpoch(seconds,0);
        }

        /**
         * @brief Construct local datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or error.
         */
        static DateTime localFromEpoch(uint32_t seconds)
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

            auto ep=(toEpochMs()<<8) | static_cast<uint8_t>(m_tz);
            return ep;
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

            auto tz=num&0xFF;
            auto epochMs=num>>8;
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

        operator Date() const noexcept
        {
            return m_date;
        }

        operator Time() const noexcept
        {
            return m_time;
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

        static int8_t m_defaultTz;
};

template <typename BufT>
void DateTime::serialize(BufT &buf, bool withMilliseconds) const
{
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
        fmt::format_to(std::back_inserter(buf),"{:+03d}:00",m_tz);
    }
}

HATN_COMMON_NAMESPACE_END

namespace fmt
{

template <>
struct formatter<HATN_COMMON_NAMESPACE::DateTime> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const HATN_COMMON_NAMESPACE::DateTime& dt, FormatContext& ctx) const
    {
        return format_to(ctx.out(),"{}",dt.toIsoString());
    }
};

}

#endif // HATNDATETIME_H
