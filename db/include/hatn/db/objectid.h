/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/objectid.h
  *
  * Contains declaration of ObjectId.
  *
  */

/****************************************************************************/

#ifndef HATNDBOBJECTID_H
#define HATNDBOBJECTID_H

#include <atomic>
#include <chrono>

#include <hatn/common/error.h>
#include <hatn/common/databuf.h>
#include <hatn/common/datetime.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT ObjectId
{
    public:

        constexpr static const uint32_t Length=24;

        static ObjectId generateId()
        {
            ObjectId id;
            id.generate();
            return id;
        }

        void generate();

        template <typename BufferT>
        void serialize(BufferT& buf)
        {
            Assert(buf.size()>=Length,"invalid buf size for ObjectId");
            fmt::format_to_n(buf.data(),Length,"{:010x}{:06x}{:08x}",m_timepoint,m_seq&0xFFFFFF,m_rand);
        }

        bool parse(const common::ConstDataBuf& buf) noexcept;

        std::string toString() const
        {
            std::array<char,Length> buf;
            serialize(buf);
            return std::string{buf.begin(),buf.end()};
        }

        common::DateTime toDatetime() const
        {
            return common::DateTime::fromEpochMs(m_timepoint);
        }

        uint64_t sinceEpochMs() const noexcept
        {
            return m_timepoint;
        }

        uint32_t seq() const noexcept
        {
            return m_seq;
        }

        uint64_t rand() const noexcept
        {
            return m_rand;
        }

        bool operator ==(const ObjectId& other) const noexcept
        {
            return m_timepoint==other.m_timepoint && m_seq==other.m_seq && m_rand==other.m_rand;
        }

        bool operator !=(const ObjectId& other) const noexcept
        {
            return !(*this==other);
        }

        bool operator <(const ObjectId& other) const noexcept
        {
            if (m_timepoint<other.m_timepoint)
            {
                return true;
            }
            if (m_timepoint>other.m_timepoint)
            {
                return false;
            }
            if (m_seq<other.m_seq)
            {
                return true;
            }
            if (m_seq>other.m_seq)
            {
                return false;
            }
            if (m_rand<other.m_rand)
            {
                return true;
            }
            if (m_rand>other.m_rand)
            {
                return false;
            }
            return false;
        }

        bool operator <=(const ObjectId& other) const noexcept
        {
            return *this==other || *this<other;
        }

        bool operator >(const ObjectId& other) const noexcept
        {
            return !(*this<=other);
        }

        bool operator >=(const ObjectId& other) const noexcept
        {
            return !(*this<other);
        }

        void reset() noexcept
        {
            m_timepoint=0;
            m_seq=0;
            m_rand=0;
        }

        bool isNull() const noexcept
        {
            return m_timepoint==0;
        }

    private:

        uint64_t m_timepoint=0; // 5 bytes: 0xFFFFFFFFFF
        uint32_t m_seq=0; // 3 bytes: 0xFFFFFF
        uint32_t m_rand=0; // 4 bytes: 0xFFFFFFFF

        // hex string: 2*(5+3+4) = 24 characters
};

HATN_DB_NAMESPACE_END

#endif // HATNDBOBJECTID_H
