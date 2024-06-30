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
thread_local std::array<char,TtlMark::Size> CurrentTimepointStr{0,0,0,0,0};
}

//---------------------------------------------------------------

uint32_t TtlMark::currentTimepoint() noexcept
{
    return CurrentTimepoint;
}

//---------------------------------------------------------------

ROCKSDB_NAMESPACE::Slice TtlMark::currentTimepointSlice() noexcept
{
    return ROCKSDB_NAMESPACE::Slice{CurrentTimepointStr.data(),CurrentTimepointStr.size()};
}

//---------------------------------------------------------------

void TtlMark::refreshCurrentTimepoint()
{
    CurrentTimepoint=common::DateTime::secondsSinceEpoch();

    uint32_t tp=boost::endian::native_to_little(CurrentTimepoint);
    memcpy(CurrentTimepointStr.data(),&tp,TtlMark::Size-1);

    CurrentTimepointStr[TtlMark::Size-1]=1;
}

//---------------------------------------------------------------

bool TtlMark::isExpired(uint32_t tp) noexcept
{
    return tp<CurrentTimepoint;
}

//---------------------------------------------------------------

bool TtlMark::isExpired(const char *data) noexcept
{
    if (data[TtlMark::Size-1]==0)
    {
        return false;
    }

    uint32_t tp=0;
    memcpy(&tp,data,TtlMark::Size-1);
    boost::endian::little_to_native_inplace(tp);

    return isExpired(tp);
}

//---------------------------------------------------------------

bool TtlMark::isExpired(const ROCKSDB_NAMESPACE::Slice* slice) noexcept
{
    if (slice->size()>=TtlMark::Size)
    {
        return isExpired(slice->data()+slice->size()-TtlMark::Size);
    }
    return false;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
