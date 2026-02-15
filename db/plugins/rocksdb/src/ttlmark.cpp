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

uint32_t TtlMark::nowTimepoint() noexcept
{
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

//---------------------------------------------------------------

uint32_t TtlMark::refreshCurrentTimepoint()
{
    CurrentTimepoint=static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    return CurrentTimepoint;
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

bool TtlMark::isExpired(uint32_t tp, uint32_t currentTp) noexcept
{
    if (currentTp==0)
    {
        auto currentTp=CurrentTimepoint;
        return tp<currentTp;
    }
    return tp<currentTp;
}

std::optional<bool> TtlMark::isExpiredOpt(const ROCKSDB_NAMESPACE::Slice& slice, uint32_t currentTp) noexcept
{
    auto tp=ttlMarkTimepoint(slice.data(),slice.size());
    if (tp==0)
    {
        return std::nullopt;
    }
    return isExpired(tp,currentTp);
}

//---------------------------------------------------------------

bool TtlMark::isExpired(const char *data, size_t size, uint32_t currentTp) noexcept
{
    auto tp=ttlMarkTimepoint(data,size);
    if (tp==0)
    {
        return false;
    }
    return isExpired(tp,currentTp);
}

//---------------------------------------------------------------

uint32_t TtlMark::ttlMarkTimepoint(const char *data, size_t size) noexcept
{
    if (size==0)
    {
        return 0;
    }
    if (data[size-1]==0)
    {
        return 0;
    }
    if (size<TtlMark::Size)
    {
        return 0;
    }

    uint32_t tp=0;
    memcpy(&tp,data+size-TtlMark::Size,TtlMark::Size-1);
    boost::endian::little_to_native_inplace(tp);

    return tp;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
