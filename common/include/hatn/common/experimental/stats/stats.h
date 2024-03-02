/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file common/stats/stats.h
 *
 *     Statistics counters.
 *
 */
/****************************************************************************/

#ifndef HATNCOMMONSTATS_H
#define HATNCOMMONSTATS_H

#include <array>

#include <hatn/common/common.h>
#include <hatn/common/objectid.h>
#include <hatn/common/types.h>
#include <hatn/common/utils.h>

HATN_COMMON_NAMESPACE_BEGIN

template <uint8_t MaxDepth=5> class StatsContainer
{
    public:

        StatsContainer(
                uint8_t depth = MaxDepth
            ) noexcept : m_pos(0),
                         m_depth(depth)
        {
            m_counters.fill(0);
        }

        void inc(uint64_t increment=1) noexcept
        {
            m_counters[m_pos]+=increment;
        }

        void set(uint64_t value) noexcept
        {
            m_counters[m_pos]=value;
        }

        void clear() noexcept
        {
            m_counters.fill(0);
        }

        const std::array<uint64_t,MaxDepth>& read() const noexcept
        {
            return m_counters;
        }

        uint8_t pos() const noexcept
        {
            return m_pos;
        }

        void switchNext() noexcept
        {
            if (m_pos==(m_depth-1))
            {
                m_pos=0;
            }
            else
            {
                ++m_pos;
            }
            set(0);
        }

        uint64_t get(uint8_t index=0) const noexcept
        {
            index+=m_pos;
            if (index>=m_depth)
            {
                index=index%m_depth;
            }

            return m_counters[index];
        }

        uint8_t depth() const noexcept
        {
            return m_depth;
        }

        constexpr static uint8_t maxDepth() noexcept
        {
            return MaxDepth;
        }

    private:

        uint8_t m_pos;
        uint8_t m_depth;
        std::array<uint64_t,MaxDepth> m_counters;
};

#if 0
template <typename T, uint8_t Depth=5> class StatsCounterType : public StatsCounter<Depth>
{
    public:

        constexpr static const char* name() noexcept
        {
            return T::name();
        }
};


template <uint8_t Depth=5> class StatsConroller
{
    public:

        StatsCounter* counter(const STR_ID_CONTENT& id) const noexcept
        {
            auto it=m_counters.find(id);
            if (it != m_counters.end())
            {
                return it.first;
            }

            return nullptr;
        }

    private:

        std::map<STR_ID_CONTENT,StatsCounter<Depth>*> m_counters;
};
#endif

using STATS_ID_TYPE = FixedByteArray32;

class StatsContext
{
    public:

        //! Default ctor.
        StatsContext() noexcept : m_parent(nullptr)
        {}

        //! Ctor.
        StatsContext(
            STATS_ID_TYPE id,
            StatsContext* parent = nullptr
        ) noexcept : m_statsId(std::move(id)),
                     m_parent(parent)
        {}

        //! Set stats ID.
        inline void setStatsID(STATS_ID_TYPE id) noexcept
        {
            m_statsId.emplace(std::move(id));
        }

        //! Get stats ID.
        inline const lib::optional<STATS_ID_TYPE>& statsId() const noexcept
        {
            return m_statsId;
        }

        //! Get stats ID as char array.
        const char* statsIdStr() const noexcept
        {
            if (m_statsId)
            {
                return m_statsId.value().c_str();
            }
            return nullptr;
        }

        //! Get parent context
        StatsContext* parent() const noexcept
        {
            return m_parent;
        }

        //! Set parent context
        void setParent(StatsContext* parent = nullptr) noexcept
        {
            m_parent=parent;
        }

        inline bool operator==(const StatsContext &rhs) const noexcept
        {
            if (!m_statsId && !rhs.m_statsId)
            {
                return true;
            }
            if (!m_statsId || !rhs.m_statsId)
            {
                return false;
            }
            return m_statsId == rhs.m_statsId;
        }

        inline bool operator<( const StatsContext &rhs ) const noexcept
        {
            return Utils::safeStrCompare(statsIdStr(),rhs.statsIdStr())<0;
        }

    private:

        lib::optional<STATS_ID_TYPE> m_statsId;
        StatsContext* m_parent;
};

using StatsFullContext = std::set<StatsContext>;

class StatsCounters
{
    public:

/*
 *
 * tenance - command -       address
 *    |         |               |
 * group     command_code    ip_address
 *    |
 * user
 *    |
 * session
 *
 *
 * */



};

HATN_COMMON_NAMESPACE_END

#endif // HATNCOMMONSTATS_H
