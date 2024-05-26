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

#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

//---------------------------------------------------------------

ObjectId ObjectId::generateId()
{
    static std::atomic<uint64_t> lastMs{0};
    static std::atomic<uint32_t> seq{0};

    ObjectId id;

    id.m_timepoint=timeSinceEpochMs();
    auto prevMs = lastMs.load();
    auto prevSeq = seq.load();
    bool restartSeq{false};

    if (id.m_timepoint>prevMs)
    {
        if (lastMs.compare_exchange_strong(id.m_timepoint,prevMs))
        {
            if (seq.compare_exchange_strong(prevSeq,1))
            {
                restartSeq=true;
            }
        }
    }

    if (!restartSeq)
    {
        id.m_seq=seq.fetch_add(1);
    }

    id.m_rand=common::Utils::uniformRand(1,0xFFFFFFFF);

    return id;
}

//---------------------------------------------------------------

std::string ObjectId::toString() const
{
    return fmt::format("{:010x}{:06x}{:08x}",m_timepoint,m_seq&0xFFFFFF,m_rand);
}

//---------------------------------------------------------------

void ObjectId::serialize(common::DataBuf &buf)
{
    Assert(buf.size()>=Length,"invalid buf size for ObjectId");
    //! @todo Use buffer wrapper to format directly to target buffer
    auto str=toString();
    memcpy(buf.data(),str.data(),str.size());
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
