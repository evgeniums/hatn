/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/ttlmark.cpp
  *
  *   Helpers for ttl with RocksDB.
  *
  */

/****************************************************************************/

#include <boost/endian/conversion.hpp>

#include <hatn/common/datetime.h>

#include <hatn/db/plugins/rocksdb/ttlmark.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace
{
thread_local uint32_t CurrentTimepoint=0;
}

//---------------------------------------------------------------

uint32_t TtlMark::currentTimepoint() noexcept
{
    return CurrentTimepoint;
}

//---------------------------------------------------------------

void TtlMark::refreshCurrentTimepoint()
{
    CurrentTimepoint=common::DateTime::secondsSinceEpoch();
}

//---------------------------------------------------------------

void TtlMark::fillExpireAt(uint32_t expireAt)
{
    if (expireAt>0)
    {
        uint32_t exp=boost::endian::native_to_little(expireAt);
        memcpy(m_expireAt.data(),&exp,TtlMark::Size-1);
        m_expireAt[TtlMark::Size-1]=1;
        m_size=5;
    }
    else
    {
        m_expireAt[0]=0;
        m_size=1;
    }
}

//---------------------------------------------------------------

bool TtlMark::isExpired(uint32_t tp) noexcept
{
    return tp<CurrentTimepoint;
}

//---------------------------------------------------------------

bool TtlMark::isExpired(const char *data, size_t size) noexcept
{
    if (size==0)
    {
        return false;
    }
    if (data[size-1]==0)
    {
        return false;
    }
    if (size<TtlMark::Size)
    {
        return false;
    }

    uint32_t tp=0;
    memcpy(&tp,data+size-TtlMark::Size,TtlMark::Size-1);
    boost::endian::little_to_native_inplace(tp);

    return isExpired(tp);
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
