/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbkeys.ipp
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
#include <hatn/db/plugins/rocksdb/ipp/fieldtostringbuf.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

constexpr static const char* NullChar="\0";
constexpr lib::string_view NullCharStr{"\0",1};

template <typename BufT=common::FmtAllocatedBufferChar>
class Keys
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

        template <typename ModelT>
        auto makeObjectKey(const ModelT& model,
                           const Namespace& ns,
                           const ROCKSDB_NAMESPACE::Slice& objectId,
                           const ROCKSDB_NAMESPACE::Slice& ttlMark
                           )
        {
            std::array<ROCKSDB_NAMESPACE::Slice,6> parts;

            parts[0]=ROCKSDB_NAMESPACE::Slice{ns.topic().data(),ns.topic().size()};
            parts[1]=ROCKSDB_NAMESPACE::Slice{NullCharStr.data(),NullCharStr.size()};
            parts[2]=ROCKSDB_NAMESPACE::Slice{model.modelIdStr().data(),model.modelIdStr().size()};
            parts[3]=ROCKSDB_NAMESPACE::Slice{NullCharStr.data(),NullCharStr.size()};
            parts[4]=objectId;
            parts[5]=ttlMark;

            return parts;
        }

        template <typename UnitT, typename IndexT>
        auto makeIndexKey(
            const Namespace& ns,
            const ROCKSDB_NAMESPACE::Slice& objectId,
            const UnitT* object,
            const IndexT& index
            )
        {
            size_t offset=m_buf.size();
            auto& buf=m_buf;

            std::array<ROCKSDB_NAMESPACE::Slice,2> key;

            buf.append(ns.topic());
            buf.append(NullCharStr);
            buf.append(index.id());
            buf.append(NullCharStr);

            auto eachField=[&object,&buf](auto&& fieldId)
            {
                const auto& field=getIndexField(*object,fieldId);
                if (field.fieldHasDefaultValue() || field.isSet())
                {
                    fieldToStringBuf(buf,field.value());
                }
                buf.append(NullCharStr);
            };
            hana::for_each(index.fields,eachField);

            key[0]=ROCKSDB_NAMESPACE::Slice{buf.data()+offset,buf.size()-offset};
            key[1]=objectId;

            return key;
        }

    private:

        BufT m_buf;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
