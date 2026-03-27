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

#include <cstdint>
#include <cmath>

class DateTime;

class DateTimePacker
{
    // Flag sits at the 8th bit of the LSB (Bit 7 of the int64)
    static constexpr int64_t EPOCH_FLAG = 1LL << 7;

    public:

        /**
         * Packs milliseconds and timezone minutes into an int64.
         * tzMinutes: Total minutes offset (e.g., +330 for India UTC+5:30)
         */
        static int64_t pack(int64_t millis, int16_t tzMinutes)
        {
            if (millis==0 && tzMinutes==0)
            {
                return EPOCH_FLAG;
            }

            // 1. Convert minutes to 15-min units to fit in 7 bits (-48 to +56)
            int8_t tzUnits = static_cast<int8_t>(tzMinutes / 15);

            // 2. Mask units to 7 bits (0x7F) and add the validity flag
            uint8_t tzPart = static_cast<uint8_t>(tzUnits) & 0x7F;
            uint8_t lsb = tzPart;

            // 3. Shift millis left by 8 and OR the LSB
            return (millis << 8) | static_cast<int64_t>(lsb);
        }

        struct Unpacked { int64_t millis; int16_t tzMinutes; };

        /**
         * Unpacks the int64. Returns empty if the flag is missing.
         */
        static std::optional<Unpacked> unpack(int64_t packed)
        {
            if (packed==0)
            {
                return std::nullopt;
            }

            // 1. Extract Millis by shifting right 8 bits
            int64_t millis = packed >> 8;

            // 2. Extract 7-bit TZ units from LSB
            uint8_t rawTz = static_cast<uint8_t>(packed & 0x7F);

            // 3. Sign-extend 7-bit to 8-bit
            if (rawTz & 0x40) { rawTz |= 0x80; }
            int8_t tzUnits = static_cast<int8_t>(rawTz);

            // 4. Convert back to minutes for the signature
            int16_t tzMinutes = static_cast<int16_t>(tzUnits) * 15;

            return Unpacked{millis, tzMinutes};
        }

        static inline int64_t packDatetime(const DateTime&);
        static inline Result<DateTime> unpackDatetime(int64_t);
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
        DateTime():m_tz(m_defaultTz)
        {}

        /**
         * @brief Ctor.
         * @param date Date.
         * @param time Time.
         * @param tz Timezone.
         */
        template <typename TzT>
        DateTime(Date date, Time time, TzT tz, bool nothrow_=false)
            :m_date(std::move(date)),m_time(std::move(time)),m_tz(static_cast<int16_t>(tz))
        {
            if (nothrow_)
            {
                if (!validate())
                {
                    reset();
                }
            }
            else
            {
                HATN_CHECK_THROW(validate())
            }
        }

        /**
         * @brief Ctor.
         * @param date Date.
         * @param time Time.
         */
        DateTime(Date date, Time time)
            : DateTime(std::move(date),std::move(time),0)
        {}

        /**
         * @brief Ctor.
         * @param date Date.
         * @param time Time.
         */
        explicit DateTime(Date date)
            : DateTime(std::move(date),Time{0,0,0,1})
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

        int32_t timezoneSeconds() const noexcept
        {
            return timezoneToSeconds(m_tz);
        }

        int16_t timezone() const noexcept
        {
            return m_tz;
        }

        static inline int32_t timezoneToSeconds(int16_t tz) noexcept
        {
            return tz*60;
        }

        /**
         * @brief Check if datetime is valid.
         * @return True if valid.
         */
        bool isValid() const noexcept
        {
            return m_date.isValid() && m_time.isValid() && isTimezoneValid(m_tz);
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
         * @param hours Hours.
         * @param Minutes minutes.
         * @return OK if tz is valid.
         *
         * This method changes timezone as is without updating time and date.
         * To convert datetime to new timezone uset toTimezone() method.
         */
        template <typename T1, typename T2=int8_t>
        Error setTimezone(T1 hours, T2 minutes) noexcept
        {
            auto tz=hours*60 + minutes;
            HATN_CHECK_RETURN(validateTz(tz))
            m_tz=static_cast<decltype(m_tz)>(tz);
            return OK;
        }

        template <typename T>
        Error setTimezone(T minutes) noexcept
        {
            HATN_CHECK_RETURN(validateTz(minutes))
            m_tz=static_cast<int16_t>(minutes);
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
            return toTimezone(*this).takeValue();
        }

        /**
         * @brief Convert datetime to local.
         * @return UTC datetime.
         */
        DateTime toLocal() const
        {
            return toTimezone(*this,localTimezone()).takeValue();
        }

        /**
         * @brief Get milliseconds since epoch.
         * @return Milliseconds since epoch.
         */
        int64_t toEpochMs() const;

        /**
         * @brief Get seconds since epoch.
         * @return Seconds since epoch.
         */
        int32_t toEpoch() const;

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
        static DateTime current(int16_t tz);

        /**
         * @brief Get current datetime.
         * @return Current datetime in default time zone.
         */
        static DateTime current();

        static void setDefaultTimezone(int16_t tz);

        static int16_t defaultTimezone() noexcept;

        void loadCurrentUtc();

        void loadCurrentLocal();

        void loadCurrent(int16_t tz);

        void loadCurrent();

        /**
         * @brief Construct datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @param tz Timezone in minutes.
         * @return Constructed datetime or throws
         * @throws In case date time is invalid
         */
        static DateTime fromEpochMs(int64_t milliseconds, int16_t tz=0);

        /**
         * @brief Construct datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @param tz Timezone in minutes.
         * @return Constructed datetime or throws.
         * @throws In case date time is invalid
         */
        static DateTime fromEpoch(int32_t seconds, int16_t tz=0);

        /**
         * @brief Construct UTC datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or throws.
         * @throws In case date time is invalid
         */
        static DateTime utcFromEpochMs(int64_t milliseconds)
        {
            return fromEpochMs(milliseconds,0);
        }

        /**
         * @brief Construct local datetime from milliseconds since epoch.
         * @param milliseconds Milliseconds since epoch.
         * @return Constructed datetime or throws.
         * @throws In case date time is invalid
         */
        static DateTime localFromEpochMs(int64_t milliseconds)
        {
            return fromEpochMs(milliseconds,localTimezone());
        }

        /**
         * @brief Construct UTC datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or throws.
         * @throws In case date time is invalid
         */
        static DateTime utcFromEpoch(int32_t seconds)
        {
            return fromEpoch(seconds,0);
        }

        /**
         * @brief Construct local datetime from seconds since epoch.
         * @param seconds Seconds since epoch.
         * @return Constructed datetime or throws.
         * @throws In case date time is invalid
         */
        static DateTime localFromEpoch(int32_t seconds)
        {
            return fromEpoch(seconds,localTimezone());
        }

        /**
         * @brief Get local timezone.
         * @return Local timezone.
         */
        static int16_t localTimezone();

        /**
         * @brief Convert datetime to datetime with given timezone.
         * @param from Original datetime.
         * @param tz Timezone in minutes.
         * @return Converted datetime or error.
         */
        static Result<DateTime> toTimezone(const DateTime& from, int16_t tz=0);

        /**
         * @brief Convert datetime to datetime with given timezone.
         * @param from Original datetime.
         * @param tz Timezone in minutes.
         * @return Converted datetime or error.
         */
        template <typename T>
        static Result<DateTime> toTimezone(const DateTime& from, T tz=0)
        {
            return toTimezone(from,static_cast<int16_t>(tz));
        }

        /**
         * @brief Validate timezone.
         * @param tz Timezone in minutes.
         * @return Validation result.
         */
        template <typename T>
        static Error validateTz(T tz)
        {
            if (!isTimezoneValid(tz))
            {
                return CommonError::INVALID_DATETIME_FORMAT;
            }
            return OK;
        }

        /**
         * @brief Get current milliseconds since epoch.
         * @return Milliseconds since epoch till now.
         */
        static int64_t millisecondsSinceEpoch();

        /**
         * @brief Get current seconds since epoch.
         * @return Seconds since epoch till now.
         */
        static int32_t secondsSinceEpoch();

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
         * @brief Add months.
         * @param value Months to add.
         */
        void addMonths(int value)
        {
            m_date.addMonths(value);
        }

        /**
         * @brief Add years.
         * @param value Years to add.
         */
        void addYears(int value)
        {
            m_date.addYears(value);
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
        int64_t toNumber() const
        {
            if (isNull())
            {
                return 0;
            }

            return DateTimePacker::packDatetime(*this);
        }

        /**
         * @brief Construct datetime from single number.
         * @param num Number.
         * @return Operation result.
         */
        static Result<DateTime> fromNumber(int64_t num)
        {
            if (num==0)
            {
                return DateTime{};
            }

            return DateTimePacker::unpackDatetime(num);
        }

        /**
         * @brief Set datetime from single number.
         * @param num Number.
         * @return Operation status.
         */
        Error setNumber(int64_t num)
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

        Error validate(const Date& date, const Time& time, int16_t tz) const noexcept
        {
            HATN_CHECK_RETURN(date.validate())
            HATN_CHECK_RETURN(time.validate())
            HATN_CHECK_RETURN(validateTz(tz))
            return OK;
        }

        std::string format(const std::locale& = {}) const;

        // Constants based on UTC-12:00 to UTC+14:00
        static constexpr int16_t TZ_MIN_OFFSET_MINUTES = -720;
        static constexpr int16_t TZ_MAX_OFFSET_MINUTES = 840;
        static bool isTimezoneValid(int16_t offsetMinutes)
        {
            // 1. Range check
            if (offsetMinutes < TZ_MIN_OFFSET_MINUTES || offsetMinutes > TZ_MAX_OFFSET_MINUTES)
            {
                return false;
            }

            // 2. Increment check (must be multiple of 15 for civil time)
            if (offsetMinutes % 15 != 0) {
                return false;
            }

            return true;
        }

    private:

        Error validate() noexcept
        {
            return validate(m_date,m_time,m_tz);
        }

        Date m_date;
        Time m_time;
        int16_t m_tz;

        static int16_t m_defaultTz;
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
        auto hours = m_tz/60;
        auto minutes = std::abs(m_tz % 60);
        fmt::format_to(std::back_inserter(buf), "{:+03d}:{:02d}", hours, minutes);
    }
}

inline int64_t DateTimePacker::packDatetime(const DateTime& dt)
{
    return pack(dt.toEpochMs(),dt.timezone());
}

inline Result<DateTime> DateTimePacker::unpackDatetime(int64_t n)
{
    auto unpacked=unpack(n);
    if (!unpacked)
    {
        return commonError(CommonError::INVALID_ARGUMENT);
    }
    return DateTime::fromEpochMs(unpacked.value().millis,unpacked.value().tzMinutes);
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
