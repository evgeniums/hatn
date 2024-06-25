/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/date.h
  *
  * Declarations of date types.
  */

/****************************************************************************/

#ifndef HATNDATE_H
#define HATNDATE_H

#include <hatn/common/common.h>
#include <hatn/common/utils.h>
#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/format.h>

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
         * @brief Serialize date to buffer.
         * @param buf Target buffer.
         * @param format Format.
         */
        template <typename BufT>
        void serialize(BufT &buf,Format format=Format::Number) const;

        /**
         * @brief Format date to string.
         * @param format Format.
         * @return Formatted date.
         */
        std::string toString(Format format=Format::Iso) const
        {
            FmtAllocatedBufferChar buf;
            serialize(buf,format);
            return fmtBufToString(buf);
        }

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

        Date copyAddDays(int days)
        {
            auto dt=*this;
            dt.addDays(days);
            return dt;
        }

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

template <typename BufT>
void Date::serialize(BufT &buf,Format format) const
{
    if (!isValid())
    {
        fmt::format_to(std::back_inserter(buf),"{}","invalid");
        return;
    }

    auto shortYear=[this]()
    {
        return m_year%100;
    };

    switch (format)
    {
    case(Format::Number):
        fmt::format_to(std::back_inserter(buf),"{:04d}{:02d}{:02d}",year(),month(),day());
        break;

    case(Format::Iso):
        fmt::format_to(std::back_inserter(buf),"{:04d}-{:02d}-{:02d}",year(),month(),day());
        break;

    case(Format::IsoSlash):
        fmt::format_to(std::back_inserter(buf),"{:04d}/{:02d}/{:02d}",year(),month(),day());
        break;

    case(Format::EuropeDot):
        fmt::format_to(std::back_inserter(buf),"{:02d}.{:02d}.{:04d}",day(),month(),year());
        break;

    case(Format::UsDot):
        fmt::format_to(std::back_inserter(buf),"{:02d}.{:02d}.{:04d}",month(),day(),year());
        break;

    case(Format::EuropeShortDot):
        fmt::format_to(std::back_inserter(buf),"{:02d}.{:02d}.{:02d}",day(),month(),shortYear());
        break;

    case(Format::UsShortDot):
        fmt::format_to(std::back_inserter(buf),"{:02d}.{:02d}.{:02d}",month(),day(),shortYear());
        break;

    case(Format::EuropeSlash):
        fmt::format_to(std::back_inserter(buf),"{:02d}/{:02d}/{:04d}",day(),month(),year());
        break;

    case(Format::UsSlash):
        fmt::format_to(std::back_inserter(buf),"{:02d}/{:02d}/{:04d}",month(),day(),year());
        break;

    case(Format::EuropeShortSlash):
        fmt::format_to(std::back_inserter(buf),"{:02d}/{:02d}/{:02d}",day(),month(),shortYear());
        break;

    case(Format::UsShortSlash):
        fmt::format_to(std::back_inserter(buf),"{:02d}/{:02d}/{:02d}",month(),day(),shortYear());
        break;

    default:
        fmt::format_to(std::back_inserter(buf),"{:04d}{:02d}{:02d}",year(),month(),day());
        break;
    }
}

HATN_COMMON_NAMESPACE_END
#endif // HATNDATE_H
