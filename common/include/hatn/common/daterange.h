/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/daterange.h
  *
  * Declarations of date range type.
  */

/****************************************************************************/

#ifndef HATNDATERANGE_H
#define HATNDATERANGE_H

#include <hatn/common/common.h>
#include <hatn/common/datetime.h>

HATN_COMMON_NAMESPACE_BEGIN

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
         * @brief Default ctor.
         */
        DateRange() : m_value(0)
        {}

        /**
         * @brief Ctor from number.
         * @param value Number.
         */
        explicit DateRange(uint32_t value) : m_value(value)
        {
            //! @todo Maybe validate.
        }

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
            :DateRange(dateToRangeNumber(dt,type))
        {}

        /**
         * @brief Check if date range is valid.
         * @return Operation restult.
         */
        bool isValid() const noexcept
        {
            return m_value!=0;
        }

        bool isNull() const noexcept
        {
            return m_value==0;
        }

        void set(uint32_t value) noexcept
        {
            //! @todo Maybe validate.
            m_value=value;
        }

        void set(const Date& dt, Type type=Type::Month)
        {
            set(dateToRangeNumber(dt,type));
        }

        void set(const DateTime& dt, Type type=Type::Month)
        {
            set(dt.date(),type);
        }

        void reset() noexcept
        {
            m_value=0;
        }

        /**
         * @brief Construct range from date.
         * @param dt Date.
         * @param type Range type.
         * @return Constructed date range.
         */
        static DateRange dateToRange(const Date& dt, Type type=Type::Month);

        /**
         * @brief Construct list of ranges for interval of dates.
         * @param to Max date.
         * @param from Min date, if null then the same as max date.
         * @param type Range type.
         * @return Set of date ranges.
         */
        static std::set<DateRange> datesToRanges(const Date& to, const Date& from=Date{}, Type type=Type::Month);

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

        template <typename BufT>
        void serialize(BufT &buf) const;

        std::string toString() const;

        static Result<DateRange> fromString(const common::lib::string_view& str);

        bool operator==(const DateRange& other) const noexcept
        {
            return m_value==other.m_value;
        }

        bool operator!=(const DateRange& other) const noexcept
        {
            return m_value!=other.m_value;
        }

        bool operator<(const DateRange& other) const noexcept
        {
            return m_value<other.m_value;
        }

        bool operator<=(const DateRange& other) const noexcept
        {
            return m_value<=other.m_value;
        }

        bool operator>(const DateRange& other) const noexcept
        {
            return m_value>other.m_value;
        }

        bool operator>=(const DateRange& other) const noexcept
        {
            return m_value>=other.m_value;
        }

    private:

        uint32_t m_value;
};

template <typename BufT>
void DateRange::serialize(BufT &buf) const
{
    fmt::format_to(std::back_inserter(buf),"{:08d}",m_value);
}

HATN_COMMON_NAMESPACE_END

namespace fmt
{
    template <>
    struct formatter<HATN_COMMON_NAMESPACE::DateRange> : formatter<string_view>
    {
        template <typename FormatContext>
        auto format(const HATN_COMMON_NAMESPACE::DateRange& dr, FormatContext& ctx) const
        {
            return format_to(ctx.out(),"{}",dr.toString());
        }
    };
}

#endif // HATNDATERANGE_H
