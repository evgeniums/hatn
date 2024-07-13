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

#include <rocksdb/db.h>

#include <hatn/common/format.h>

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

        template <typename ModelT>
        static auto makeObjectKey(const ModelT& model,
                                  const Namespace& ns,
                                  const ROCKSDB_NAMESPACE::Slice& objectId,
                                  const ROCKSDB_NAMESPACE::Slice& ttlMark=ROCKSDB_NAMESPACE::Slice{}
                                  ) noexcept
        {
            //! @todo add timestamp
            std::array<ROCKSDB_NAMESPACE::Slice,6> parts;

            parts[0]=ROCKSDB_NAMESPACE::Slice{ns.topic().data(),ns.topic().size()};
            parts[1]=ROCKSDB_NAMESPACE::Slice{SeparatorCharStr.data(),SeparatorCharStr.size()};
            parts[2]=ROCKSDB_NAMESPACE::Slice{model.modelIdStr().data(),model.modelIdStr().size()};
            parts[3]=ROCKSDB_NAMESPACE::Slice{SeparatorCharStr.data(),SeparatorCharStr.size()};
            parts[4]=objectId;

            // ttl mark is used only for data of indexes, actual object key must use only [0:4] parts
            parts[5]=ttlMark;

            return parts;
        }

        static ROCKSDB_NAMESPACE::Slice objectKeyFromIndexValue(const char* ptr, size_t size)
        {
            Assert(size>TtlMark::Size,"Invalid size of index value");
            //! @todo omit timestamp
            ROCKSDB_NAMESPACE::Slice r{ptr,size-TtlMark::Size};
            return r;
        }

        static ROCKSDB_NAMESPACE::SliceParts objectKeySlices(const std::array<ROCKSDB_NAMESPACE::Slice,6>& parts) noexcept
        {
            // -1 because the last element of objectKey is a ttl mark
            return ROCKSDB_NAMESPACE::SliceParts{&parts[0],static_cast<int>(parts.size()-1)};
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

        ROCKSDB_NAMESPACE::Slice objectKeySlice(const std::array<ROCKSDB_NAMESPACE::Slice,6>& parts)
        {
            reset();
            for (size_t i=0;i<parts.size()-2;i++)
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
                    return handler(key);
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
            const Namespace& ns,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index,
            const KeyHandlerT& handler
            )
        {
            size_t offset=m_buf.size();

            m_buf.append(ns.topic());
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
