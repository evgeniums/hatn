/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbkeys.ipp
  *
  *   RocksDB keys processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBKEYS_IPP
#define HATNROCKSDBKEYS_IPP

#include <boost/endian/conversion.hpp>

#include <rocksdb/db.h>

#include <hatn/common/format.h>
#include <hatn/common/datetime.h>

#include <hatn/db/objectid.h>
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

using IndexKeySlice=std::array<ROCKSDB_NAMESPACE::Slice,2>;

class KeysBase
{
    public:

        constexpr static const size_t ObjectKeySliceCount=5;
        constexpr static const size_t TimestampSize=4;

        inline static const char ObjectIndexVersion{0x1};

        template <typename ModelT>
        static auto makeObjectKeyValue(const ModelT& model,
                                       const lib::string_view& topic,
                                       const ROCKSDB_NAMESPACE::Slice& objectId,
                                       const common::DateTime& timepoint,
                                       const TtlMark& ttlMark
                                       ) noexcept
        {
            return makeObjectKeyValue(model,topic,objectId,timepoint,ttlMark.slice());
        }

        template <typename ModelT>
        static auto makeObjectKeyValue(const ModelT& model,
                                  const lib::string_view& topic,
                                  const ROCKSDB_NAMESPACE::Slice& objectId,
                                  const common::DateTime& timepoint=common::DateTime{},
                                  const ROCKSDB_NAMESPACE::Slice& ttlMark=ROCKSDB_NAMESPACE::Slice{}
                                  ) noexcept
        {
            auto r=std::make_tuple(std::array<ROCKSDB_NAMESPACE::Slice,8>{},uint32_t{0});
            auto& parts=std::get<0>(r);

            // first byte is version of index format
            parts[0]=ROCKSDB_NAMESPACE::Slice{&ObjectIndexVersion,sizeof(ObjectIndexVersion)};

            // actual index
            parts[1]=ROCKSDB_NAMESPACE::Slice{topic.data(),topic.size()};
            parts[2]=ROCKSDB_NAMESPACE::Slice{SeparatorCharStr.data(),SeparatorCharStr.size()};
            parts[3]=ROCKSDB_NAMESPACE::Slice{model.modelIdStr().data(),model.modelIdStr().size()};
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

        static ROCKSDB_NAMESPACE::Slice objectKeyFromIndexValue(const char* ptr, size_t size)
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

        static ROCKSDB_NAMESPACE::Slice objectIdFromIndexValue(const char* ptr, size_t size)
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

        static uint32_t timestampFromIndexValue(const char* ptr, size_t size)
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
};

class Keys : public KeysBase
{
    public:

        constexpr static const size_t PreallocatedBufferSize=512;

        Keys(AllocatorFactory* allocatorFactory=common::pmr::AllocatorFactory::getDefault())
            : m_allocatorFactory(allocatorFactory),
              m_bufs(allocatorFactory->dataAllocator<BufT>())
        {}

        void reset()
        {
            m_bufs.clear();
        }

        BufT& addBuf()
        {
            m_bufs.emplace_back(m_allocatorFactory->bytesAllocator());
            m_bufs.back().reserve(PreallocatedBufferSize);
            return m_bufs.back();
        }

        template <size_t Size>
        ROCKSDB_NAMESPACE::Slice objectKeySolid(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& parts)
        {
            auto& buf=addBuf();
            for (size_t i=1;i<ObjectKeySliceCount+1;i++)
            {
                buf.append(parts[i]);
            }
            return ROCKSDB_NAMESPACE::Slice{buf.data(),buf.size()};
        }

        template <typename UnitT, typename IndexT, typename KeyHandlerT>
        Error makeIndexKey(
            const lib::string_view& topic,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerT& handler
            )
        {
            auto& buf=addBuf();

            buf.append(topic);
            buf.append(SeparatorCharStr);
            buf.append(index.id());
            buf.append(SeparatorCharStr);

            return iterateIndexFields(buf,objectId,object,index,handler,hana::size_c<0>);
        }

    private:

        AllocatorFactory* m_allocatorFactory;
        common::pmr::vector<BufT> m_bufs;

        template <typename UnitT, typename IndexT, typename KeyHandlerT, typename PosT>
        Error iterateIndexFields(
            BufT& buf,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerT& handler,
            const PosT& pos
            )
        {
            auto self=this;
            return hana::eval_if(
                hana::equal(pos,hana::size(index.fields)),
                [&](auto _)
                {
                    IndexKeySlice key;
                    key[0]=ROCKSDB_NAMESPACE::Slice{_(buf).data(),_(buf).size()};
                    key[1]=_(objectId);
                    return _(handler)(key);
                },
                [&](auto _)
                {
                    const auto& fieldId=hana::at(_(index).fields,_(pos));
                    const auto& field=getIndexField(*_(object),fieldId);

                    using fieldT=std::decay_t<decltype(field)>;
                    using repeatedT=typename fieldT::isRepeatedType;

                    return hana::eval_if(
                        repeatedT{},
                        [&](auto _)
                        {
                            if (_(field).isSet())
                            {
                                for (size_t i=0;i<_(field).count();i++)
                                {
                                    const auto& val=_(field).at(i);
                                    fieldToStringBuf(_(buf),val);
                                    _(buf).append(SeparatorCharStr);
                                    auto ec=_(self)->iterateIndexFields(
                                        _(buf),
                                        _(objectId),
                                        _(object),
                                        _(index),
                                        _(handler),
                                        hana::plus(_(pos),hana::size_c<1>)
                                        );
                                    HATN_CHECK_EC(ec)
                                }
                            }
                            else
                            {
                                _(buf).append(SeparatorCharStr);
                                return _(self)->iterateIndexFields(
                                    _(buf),
                                    _(objectId),
                                    _(object),
                                    _(index),
                                    _(handler),
                                    hana::plus(_(pos),hana::size_c<1>)
                                    );
                            }
                        },
                        [&](auto _)
                        {
                            if (_(field).fieldHasDefaultValue() || _(field).isSet())
                            {
                                fieldToStringBuf(_(buf),_(field).value());
                            }
                            _(buf).append(SeparatorCharStr);
                            return _(self)->iterateIndexFields(
                                _(buf),
                                _(objectId),
                                _(object),
                                _(index),
                                _(handler),
                                hana::plus(_(pos),hana::size_c<1>)
                                );
                        }
                        );
                }
            );
        }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
