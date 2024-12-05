/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/time.h
  *
  * Declarations of ime types.
  */

/****************************************************************************/

#ifndef HATNTIME_H
#define HATNTIME_H

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/format.h>

HATN_COMMON_NAMESPACE_BEGIN

class DateTime;

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
        explicit Time(uint64_t value)
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
         * @brief Serialize date to buffer.
         * @param precision Format precision.
         * @param ampm True if AM/PM mode must be used.
         */
        template <typename BufT>
        void serialize(
            BufT &buf,
            FormatPrecision precision=FormatPrecision::Second,
            bool ampm=false
        ) const;

        /**
         * @brief Format time to string.
         * @param precision Format precision.
         * @param ampm True if AM/PM mode must be used.
         * @return Formatted time.
         */
        std::string toString(
            FormatPrecision precision=FormatPrecision::Second,
            bool ampm=false
        ) const
        {
            FmtAllocatedBufferChar buf;
            serialize(buf,precision,ampm);
            return fmtBufToString(buf);
        }

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

template <typename BufT>
void Time::serialize(BufT &buf, FormatPrecision precision, bool ampm) const
{
    if (!isValid())
    {
        fmt::format_to(std::back_inserter(buf),"{}","invalid");
        return;
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
            fmt::format_to(std::back_inserter(buf),"{}:{:02d}:{:02d} {}",hour,m_minute,m_second,ampmStr);
            break;

        case (FormatPrecision::Minute):
            fmt::format_to(std::back_inserter(buf),"{}:{:02d} {}",hour,m_minute,ampmStr);
            break;

        case (FormatPrecision::Millisecond):
            fmt::format_to(std::back_inserter(buf),"{}:{:02d}:{:02d}.{:03d} {}",hour,m_minute,m_second,m_millisecond,ampmStr);
            break;

        case (FormatPrecision::Number):
            fmt::format_to(std::back_inserter(buf),"{}{:02d}{:02d}.{:03d} {}",hour,m_minute,m_second,m_millisecond,ampmStr);
            break;
        }
    }
    else
    {
        switch (precision)
        {
        case (FormatPrecision::Second):
            fmt::format_to(std::back_inserter(buf),"{:02d}:{:02d}:{:02d}",m_hour,m_minute,m_second);
            break;

        case (FormatPrecision::Minute):
            fmt::format_to(std::back_inserter(buf),"{:02d}:{:02d}",m_hour,m_minute);
            break;

        case (FormatPrecision::Millisecond):
            if (m_millisecond>0)
            {
                fmt::format_to(std::back_inserter(buf),"{:02d}:{:02d}:{:02d}.{:03d}",m_hour,m_minute,m_second,m_millisecond);
            }
            else
            {
                fmt::format_to(std::back_inserter(buf),"{:02d}:{:02d}:{:02d}",m_hour,m_minute,m_second);
            }
            break;

        case (FormatPrecision::Number):
            if (m_millisecond>0)
            {
                fmt::format_to(std::back_inserter(buf),"{:02d}{:02d}{:02d}.{:03d}",m_hour,m_minute,m_second,m_millisecond);
            }
            else
            {
                fmt::format_to(std::back_inserter(buf),"{:02d}{:02d}{:02d}",m_hour,m_minute,m_second);
            }
            break;
        }
    }
}

HATN_COMMON_NAMESPACE_END

namespace fmt
{
    template <>
    struct formatter<HATN_COMMON_NAMESPACE::Time> : formatter<string_view>
    {
        template <typename FormatContext>
        auto format(const HATN_COMMON_NAMESPACE::Time& t, FormatContext& ctx) const
        {
            return format_to(ctx.out(),"{}",t.toString());
        }
    };
}

#endif // HATNTIME_H
