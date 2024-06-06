/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/objectid.cpp
  *
  *   Definition of ObjectId.
  *
  */

/****************************************************************************/

#if __cplusplus >= 201703L
#include <charconv>
#endif

#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

//---------------------------------------------------------------

void ObjectId::generate()
{
    static std::atomic<uint64_t> lastMs{0};
    static std::atomic<uint32_t> seq{0};

    m_timepoint=common::DateTime::millisecondsSinceEpoch();
    auto prevMs=lastMs.load();
    auto prevSeq=seq.load();
    bool restartSeq{false};

    if (m_timepoint>prevMs)
    {
        if (lastMs.compare_exchange_strong(m_timepoint,prevMs))
        {
            if (seq.compare_exchange_strong(prevSeq,1))
            {
                restartSeq=true;
            }
        }
    }

    if (!restartSeq)
    {
        m_seq=seq.fetch_add(1);
    }

    m_rand=common::Utils::uniformRand(1,0xFFFFFFFF);
}

//---------------------------------------------------------------

bool ObjectId::parse(const common::ConstDataBuf &buf) noexcept
{
    if (buf.size()!=Length)
    {
        reset();
        return false;
    }

#if __cplusplus < 201703L

    static_assert(false,"C++14 not supported for db");

#else

    auto r = std::from_chars(buf.data(), buf.data() + 10, m_timepoint, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }
    r = std::from_chars(r.ptr, r.ptr + 6, m_seq, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }
    r = std::from_chars(r.ptr, r.ptr + 8, m_rand, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }

#endif

    return true;
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
