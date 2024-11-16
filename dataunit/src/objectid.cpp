/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file dataunit/objectid.cpp
  *
  *   Definition of ObjectId.
  *
  */

/****************************************************************************/

#if __cplusplus >= 201703L
#include <charconv>
#endif

#include <hatn/dataunit/objectid.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

void ObjectId::generate()
{
    static std::atomic<uint64_t> lastMs{0};
    static std::atomic<uint32_t> seq{1};

    m_timepoint=common::DateTime::millisecondsSinceEpoch();
    auto prevMs=lastMs.load();
    auto prevSeq=seq.load();
    bool restartSeq{false};

    if (m_timepoint>prevMs)
    {
        if (lastMs.compare_exchange_strong(prevMs,m_timepoint))
        {
            if (seq.compare_exchange_strong(prevSeq,1))
            {
                restartSeq=true;
            }
        }
    }

    if (!restartSeq)
    {
        m_seq=seq.fetch_add(1)+1;
    }
    else
    {
        m_seq=1;
    }

    m_rand=common::Random::uniform(1,0xFFFFFFFF);
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
    static_assert(false,"Not implemented yet");
    try
    {
        std::string dtStr{buf.data(), DateTimeLength};
        m_timepoint=std::stoll(dtStr,nullptr,16);
    }
    catch(...)
    {
        return false;
    }
#else

    auto r = std::from_chars(buf.data(), buf.data() + DateTimeLength, m_timepoint, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }
    r = std::from_chars(r.ptr, r.ptr + SeqLength, m_seq, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }
    r = std::from_chars(r.ptr, r.ptr + RandLength, m_rand, 16);
    if (r.ec != std::errc())
    {
        reset();
        return false;
    }

#endif

    return true;
}

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END
