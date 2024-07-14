/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/timepointfilter.h
  *
  * Contains timepoint filter.
  *
  */

/****************************************************************************/

#ifndef HATNTPFILTER_H
#define HATNTPFILTER_H

#include <hatn/common/meta/decaytuple.h>

#include <hatn/db/db.h>
#include <hatn/db/query.h>
#include <hatn/db/object.h>

HATN_DB_NAMESPACE_BEGIN

using TimePointInterval=query::Interval<uint32_t>;

using TimepointIntervals=common::pmr::vector<TimePointInterval>;
using TimepointIntervalsPtr=common::SharedPtr<TimepointIntervals>;

class TimePointFilter
{
    public:

        TimePointFilter()=default;

        TimePointFilter(TimepointIntervalsPtr intervals) : m_intervals(std::move(intervals))
        {}

        void setFilterTimePoints(TimepointIntervalsPtr intervals)
        {
            m_intervals=std::move(intervals);
        }

        const auto& getFilterTimePoints() const noexcept
        {
            return m_intervals;
        }

        bool filterTimePoint(uint32_t timepoint) const noexcept
        {
            if (m_intervals)
            {
                for (const auto& tpInterval : *m_intervals)
                {
                    if (tpInterval.contains(timepoint))
                    {
                        // filtered
                        return true;
                    }
                }
            }

            // passed
            return false;
        }

        template <typename UnitT>
        bool filterTimePoint(const UnitT& unit) const noexcept
        {
            const auto& createdAt=unit.field(object::created_at).value();
            return filterTimepoint(createdAt.toEpoch());
        }

    private:

        TimepointIntervalsPtr m_intervals;
};

HATN_DB_NAMESPACE_END

#endif // HATNTPFILTER_H
