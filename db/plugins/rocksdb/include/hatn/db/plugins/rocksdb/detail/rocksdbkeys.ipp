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

using IndexKeyT=std::array<ROCKSDB_NAMESPACE::Slice,2>;

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
                                  const common::DateTime& timepoint=common::DateTime{},
                                  const ROCKSDB_NAMESPACE::Slice& ttlMark=ROCKSDB_NAMESPACE::Slice{}
                                  ) noexcept
        {
            std::array<ROCKSDB_NAMESPACE::Slice,8> parts;

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
                uint32_t timestamp=timepoint.toEpoch();
                boost::endian::native_to_little_inplace(timestamp);
                parts[6]=ROCKSDB_NAMESPACE::Slice{reinterpret_cast<const char*>(&timestamp),TimestampSize};

                // ttl mark is the last slice
                parts[7]=ttlMark;
            }

            // done
            return parts;
        }

        template <size_t Size>
        static ROCKSDB_NAMESPACE::SliceParts objectKeySlices(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& parts) noexcept
        {
            return ROCKSDB_NAMESPACE::SliceParts{&parts[1],ObjectKeySliceCount};
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

template <typename BufT=common::FmtAllocatedBufferChar>
class Keys : public KeysBase
{
    public:

        using bufT=BufT;

        Keys()
        {}

        template <typename AllocatorT>
        Keys(const AllocatorT& alloc):m_buf(alloc)
        {}

        void reset()
        {
            m_buf.clear();
        }

        const bufT& buf() const noexcept
        {
            return m_buf;
        }

        template <size_t Size>
        ROCKSDB_NAMESPACE::Slice objectKeySolid(const std::array<ROCKSDB_NAMESPACE::Slice,Size>& parts)
        {
            reset();
            for (size_t i=1;i<ObjectKeySliceCount+1;i++)
            {
                m_buf.append(parts[i]);
            }
            return ROCKSDB_NAMESPACE::Slice{m_buf.data(),m_buf.size()};
        }

        template <typename UnitT, typename IndexT, typename KeyHandlerT, typename PosT>
        Error iterateIndexFields(
                const ROCKSDB_NAMESPACE::Slice& objectId,
                const UnitT* object,
                const IndexT& index,
                const KeyHandlerT& handler,
                size_t offset,
                const PosT& pos
            )
        {
            auto self=this;
            return hana::eval_if(
                hana::equal(pos,hana::size(index.fields)),
                [&](auto _)
                {
                    IndexKeyT key;
                    key[0]=ROCKSDB_NAMESPACE::Slice{_(self)->m_buf.data()+_(offset),_(self)->m_buf.size()-_(offset)};
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
                                auto fieldOffset=_(self)->m_buf.size();
                                for (size_t i=0;i<_(field).count();i++)
                                {
                                    _(self)->m_buf.resize(fieldOffset);
                                    const auto& val=_(field).at(i);
                                    fieldToStringBuf(_(self)->m_buf,val);
                                    _(self)->m_buf.append(SeparatorCharStr);
                                    auto ec=_(self)->iterateIndexFields(
                                        _(objectId),
                                        _(object),
                                        _(index),
                                        _(handler),
                                        _(offset),
                                        hana::plus(_(pos),hana::size_c<1>)
                                        );
                                    HATN_CHECK_EC(ec)
                                }
                            }
                            else
                            {
                                _(self)->m_buf.append(SeparatorCharStr);
                                return _(self)->iterateIndexFields(
                                    _(objectId),
                                    _(object),
                                    _(index),
                                    _(handler),
                                    _(offset),
                                    hana::plus(_(pos),hana::size_c<1>)
                                    );
                            }
                        },
                        [&](auto _)
                        {
                            if (_(field).fieldHasDefaultValue() || _(field).isSet())
                            {
                                fieldToStringBuf(_(self)->m_buf,_(field).value());
                            }
                            _(self)->m_buf.append(SeparatorCharStr);
                            return _(self)->iterateIndexFields(
                                _(objectId),
                                _(object),
                                _(index),
                                _(handler),
                                _(offset),
                                hana::plus(_(pos),hana::size_c<1>)
                                );
                        }
                    );
                }
            );
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
            size_t offset=m_buf.size();

            m_buf.append(topic);
            m_buf.append(SeparatorCharStr);
            m_buf.append(index.id());
            m_buf.append(SeparatorCharStr);

            return iterateIndexFields(objectId,object,index,handler,offset,hana::size_c<0>);
        }

    private:

        BufT m_buf;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
