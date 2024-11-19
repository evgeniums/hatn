/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbkeys.h
  *
  *   RocksDB keys processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBKEYS_H
#define HATNROCKSDBKEYS_H

#include <rocksdb/db.h>

#include <hatn/common/allocatoronstack.h>
#include <hatn/common/format.h>
#include <hatn/common/datetime.h>

#include <hatn/db/objectid.h>
#include <hatn/db/topic.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

using IndexKeySlice=std::array<ROCKSDB_NAMESPACE::Slice,2>;

class HATN_ROCKSDB_SCHEMA_EXPORT Keys
{
    public:

        using KeyBufT=common::pmr::string;

        using ObjectKeyValue=std::tuple<std::array<ROCKSDB_NAMESPACE::Slice,8>,uint32_t>;

        constexpr static const size_t PreallocatedBuffersCount=8;

        constexpr static const size_t ObjectKeySliceCount=5;
        constexpr static const size_t TimestampSize=4;

        inline static const char ObjectIndexVersion{0x1};

        using KeyHandlerFn=std::function<Error (const IndexKeySlice&)>;

        Keys(AllocatorFactory* factory) : factory(factory)
        {}

        void reset()
        {
            m_bufs.clear();
        }

        KeyBufT& addBuf()
        {
            m_bufs.emplace_back(factory->bytesAllocator());
            auto& buf=m_bufs.back();
            buf.reserve(PreallocatedKeySize);
            return buf;
        }

        template <size_t Size>
        ROCKSDB_NAMESPACE::Slice objectKeySolid(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& parts)
        {
            auto& buf=addBuf();
            for (size_t i=1;i<ObjectKeySliceCount+1;i++)
            {
                const auto& p=parts[i];
                buf.append(p.data(),p.size());
            }
            return ROCKSDB_NAMESPACE::Slice{buf.data(),buf.size()};
        }

        template <typename UnitT, typename IndexT>
        Error makeIndexKey(
            const lib::string_view& topic,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerFn& handler
            )
        {
            auto& buf=addBuf();

            buf.append(topic);
            buf.append(SeparatorCharStr);
            buf.append(index.id());
            buf.append(SeparatorCharStr);

            return iterateIndexFields(buf,objectId,object,index,handler,hana::size_c<0>);
        }

        static ObjectKeyValue makeObjectKeyValue(const std::string& modelId,
                                       const lib::string_view& topic,
                                       const ROCKSDB_NAMESPACE::Slice& objectId,
                                       const common::DateTime& timepoint,
                                       const TtlMark& ttlMark
                                       ) noexcept
        {
            return makeObjectKeyValue(modelId,topic,objectId,timepoint,ttlMark.slice());
        }

        static ObjectKeyValue makeObjectKeyValue(const std::string& modelId,
                                       const lib::string_view& topic,
                                       const ROCKSDB_NAMESPACE::Slice& objectId,
                                       const common::DateTime& timepoint=common::DateTime{},
                                       const ROCKSDB_NAMESPACE::Slice& ttlMark=ROCKSDB_NAMESPACE::Slice{}
                                       ) noexcept;

        template <size_t Size>
        static ROCKSDB_NAMESPACE::SliceParts objectKeySlices(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& objectKeyFull) noexcept
        {
            return ROCKSDB_NAMESPACE::SliceParts{&objectKeyFull[1],ObjectKeySliceCount};
        }

        template <size_t Size>
        static ROCKSDB_NAMESPACE::SliceParts indexValueSlices(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& objectKeyFull) noexcept
        {
            return ROCKSDB_NAMESPACE::SliceParts{&objectKeyFull[0],static_cast<int>(objectKeyFull.size())};
        }

        static ROCKSDB_NAMESPACE::Slice objectKeyFromIndexValue(const char* ptr, size_t size);

        static ROCKSDB_NAMESPACE::Slice objectKeyFromIndexValue(const ROCKSDB_NAMESPACE::Slice& val)
        {
            return objectKeyFromIndexValue(val.data(),val.size());
        }

        static ROCKSDB_NAMESPACE::Slice objectIdFromIndexValue(const char* ptr, size_t size);

        static uint32_t timestampFromIndexValue(const char* ptr, size_t size);

    private:

        common::VectorOnStackT<KeyBufT,PreallocatedBuffersCount> m_bufs;
        AllocatorFactory* factory;

        template <typename UnitT, typename IndexT, typename PosT, typename BufT>
        Error iterateIndexFields(
            BufT& buf,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerFn& handler,
            const PosT& pos
        );
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
