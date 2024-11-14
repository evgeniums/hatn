/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbkeys.cpp
  *
  *   RocksDB keys processing.
  *
  */

/****************************************************************************/

#include <boost/endian/conversion.hpp>

#include <rocksdb/db.h>

#include <hatn/common/format.h>
#include <hatn/common/datetime.h>

#include <hatn/db/objectid.h>
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

Keys::ObjectKeyValue Keys::makeObjectKeyValue(const std::string& modelId,
                               const lib::string_view& topic,
                               const ROCKSDB_NAMESPACE::Slice& objectId,
                               const common::DateTime& timepoint,
                               const ROCKSDB_NAMESPACE::Slice& ttlMark
                               ) noexcept
{
    auto r=std::make_tuple(std::array<ROCKSDB_NAMESPACE::Slice,8>{},uint32_t{0});
    auto& parts=std::get<0>(r);

    // first byte is version of index format
    parts[0]=ROCKSDB_NAMESPACE::Slice{&ObjectIndexVersion,sizeof(ObjectIndexVersion)};

    // actual index
    parts[1]=ROCKSDB_NAMESPACE::Slice{topic.data(),topic.size()};
    parts[2]=ROCKSDB_NAMESPACE::Slice{SeparatorCharStr.data(),SeparatorCharStr.size()};
    parts[3]=ROCKSDB_NAMESPACE::Slice{modelId.data(),modelId.size()};
    parts[4]=ROCKSDB_NAMESPACE::Slice{SeparatorCharStr.data(),SeparatorCharStr.size()};
    parts[5]=objectId;

    //! @note timepoint and ttl mark are empty ONLY when key is generated for search
    if (!timepoint.isNull() && !ttlMark.empty())
    {
        // 4 bytes for current timestamp
        uint32_t& timestamp=std::get<1>(r);
        timestamp=timepoint.toEpoch();
        boost::endian::native_to_little_inplace(timestamp);
        parts[6]=ROCKSDB_NAMESPACE::Slice{reinterpret_cast<const char*>(&timestamp),TimestampSize};

        // ttl mark is the last slice
        parts[7]=ttlMark;
    }

    // done
    return r;
}

ROCKSDB_NAMESPACE::Slice Keys::objectKeyFromIndexValue(const char* ptr, size_t size)
{
    auto extraSize=sizeof(ObjectIndexVersion)
                     + TimestampSize
                     + TtlMark::ttlMarkOffset(ptr,size);
    if (size<extraSize)
    {
        return ROCKSDB_NAMESPACE::Slice{};
    }

    ROCKSDB_NAMESPACE::Slice result{ptr+sizeof(ObjectIndexVersion),size - extraSize};
    return result;
}

ROCKSDB_NAMESPACE::Slice Keys::objectIdFromIndexValue(const char* ptr, size_t size)
{
    auto extraSize=TimestampSize
                     + TtlMark::ttlMarkOffset(ptr,size);
    if (size<(extraSize+ObjectId::Length))
    {
        return ROCKSDB_NAMESPACE::Slice{};
    }

    ROCKSDB_NAMESPACE::Slice result{ptr+size-extraSize-ObjectId::Length,ObjectId::Length};
    return result;
}

uint32_t Keys::timestampFromIndexValue(const char* ptr, size_t size)
{
    auto extraSize=TimestampSize
                     + TtlMark::ttlMarkOffset(ptr,size);
    if (size<extraSize)
    {
        return 0;
    }

    size_t offset=size-extraSize;
    uint32_t timestamp{0};
    memcpy(&timestamp,ptr+offset,sizeof(uint32_t));
    boost::endian::little_to_native_inplace(timestamp);
    return timestamp;
}

HATN_ROCKSDB_NAMESPACE_END
