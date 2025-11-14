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

        enum class IsIndexSet : uint8_t
        {
            Unknown,
            Yes,
            No
        };

        using KeyBufT=common::pmr::string;

        using ObjectKeyValue=std::array<ROCKSDB_NAMESPACE::Slice,8>;

        constexpr static const size_t PreallocatedBuffersCount=8;

        constexpr static const size_t ObjectKeySliceCount=5;
        constexpr static const size_t TimestampSize=4;

        inline static const char ObjectIndexVersion{0x1};

        using KeyHandlerFn=std::function<Error (const IndexKeySlice&, IsIndexSet)>;

        Keys(const AllocatorFactory* factory) : factory(factory)
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
            return objectKeySolid(addBuf(),parts);
        }

        template <typename BufT, size_t Size>
        static ROCKSDB_NAMESPACE::Slice objectKeySolid(BufT& buf, const std::array<ROCKSDB_NAMESPACE::Slice,Size>& parts)
        {
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
                                       uint32_t* timestamp,
                                       const common::DateTime& timepoint,
                                       const TtlMark& ttlMark
                                       ) noexcept
        {
            return makeObjectKeyValue(modelId,topic,objectId,timestamp,timepoint,ttlMark.slice());
        }

        static ObjectKeyValue makeObjectKeyValue(const std::string& modelId,
                                       const lib::string_view& topic,
                                       const ROCKSDB_NAMESPACE::Slice& objectId,
                                       uint32_t* timestamp=nullptr,
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
        const AllocatorFactory* factory;

        template <typename UnitT, typename IndexT, typename PosT, typename BufT>
        Error iterateIndexFields(
            BufT& buf,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerFn& handler,
            PosT pos,
            IsIndexSet=IsIndexSet::Unknown
        );
};

HATN_ROCKSDB_NAMESPACE_END

namespace fmt
{
    template <>
    struct formatter<ROCKSDB_NAMESPACE::Slice> : formatter<string_view>
    {
        template <typename FormatContext>
        auto format(const ROCKSDB_NAMESPACE::Slice& s, FormatContext& ctx) const
        {
            auto it=ctx.out();
            for (size_t i=0;i<s.size();i++)
            {
                auto ch=*(s.data()+i);
                if (ch<=HATN_ROCKSDB_NAMESPACE::SpaceCharC)
                {
                    it=format_to(it,"\\{:d}",ch);
                }
                else
                {
                    it=format_to(it,"{}",ch);
                }
            }
            return it;
        }
    };
}

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename T>
KeyBuf logKey(const T& key)
{
    KeyBuf buf;
    buf.reserve(key.size()+10);
    for (size_t i=0;i<key.size();i++)
    {
        auto ch=*(key.data()+i);
        if (uint8_t(ch)<uint8_t(SpaceCharC) || uint8_t(ch)>uint8_t(AsciiMaxPrintableCharC))
        {
            buf.push_back(BackSlashCharC);
            switch (ch)
            {
                case (SeparatorCharC): buf.append("0"); break;
                // case (DbNullCharC): buf.append("1"); break;
                case (EmptyCharC): buf.append("2"); break;
                case (SeparatorCharPlusC): buf.append("F"); break;
                case (InternalPrefixC): buf.append("FF"); break;
                default: buf.append(fmt::format("{:02x}",ch));
            }
        }
        else
        {
            buf.push_back(ch);
        }
    }
    return buf;
}

inline KeyBuf logKey(const ROCKSDB_NAMESPACE::SliceParts& key)
{
    KeyBuf buf;
    size_t size=0;
    for (int i=0;i<key.num_parts;i++)
    {
        size+=key.parts[i].size();
    }
    if (size>0)
    {
        buf.reserve(size);
    }
    for (int i=0;i<key.num_parts;i++)
    {
        buf.append(key.parts[i].data(),key.parts[i].size());
    }

    if (!buf.empty())
    {
        return logKey(buf);
    }

    return buf;
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
