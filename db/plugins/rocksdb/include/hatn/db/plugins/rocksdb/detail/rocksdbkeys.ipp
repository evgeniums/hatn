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
#include <hatn/common/datetime.h>

#include <hatn/db/objectid.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp>

#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename UnitT, typename IndexT, typename PosT, typename BufT>
Error Keys::iterateIndexFields(
        BufT& buf,
        const ROCKSDB_NAMESPACE::Slice& objectId,
        const UnitT* object,
        const IndexT& index,
        const Keys::KeyHandlerFn& handler,
        PosT pos
    )
{
    if constexpr (decltype(hana::equal(pos,hana::size(index.fields)))::value)
    {
        IndexKeySlice key;
        key[0]=ROCKSDB_NAMESPACE::Slice{buf.data(),buf.size()};
        key[1]=objectId;
        return handler(key);
    }
    else
    {
        const auto& fieldId=hana::at(index.fields,pos);
        const auto& field=getIndexField(*object,fieldId);

        using fieldT=std::decay_t<decltype(field)>;
        using repeatedT=typename fieldT::isRepeatedType;

        if constexpr (repeatedT::value)
        {
            if (field.isSet())
            {
                for (size_t i=0;i<field.count();i++)
                {
                    auto sizeBefore=buf.size();

                    const auto& val=field.at(i);
                    if constexpr (std::is_base_of<du::detail::BytesTraitsBase,std::decay_t<decltype(val)>>::value)
                    {
                        fieldToStringBuf(buf,val.stringView());
                    }
                    else
                    {
                        fieldToStringBuf(buf,val);
                    }

                    buf.append(SeparatorCharStr);

                    auto ec=iterateIndexFields(
                        buf,
                        objectId,
                        object,
                        index,
                        handler,
                        hana::plus(pos,hana::size_c<1>)
                        );
                    HATN_CHECK_EC(ec)

                    buf.resize(sizeBefore);
                }
                return Error{OK};
            }
            else
            {
                buf.append(SeparatorCharStr);
                return iterateIndexFields(
                    buf,
                    objectId,
                    object,
                    index,
                    handler,
                    hana::plus(pos,hana::size_c<1>)
                );
            }
        }
        else
        {
            if (field.fieldHasDefaultValue() || field.isSet())
            {
                fieldToStringBuf(buf,field.value());
            }
            else
            {
                // append Null if value not set
                fieldToStringBuf(buf,query::Null);
            }
            buf.append(SeparatorCharStr);
            return iterateIndexFields(
                buf,
                objectId,
                object,
                index,
                handler,
                hana::plus(pos,hana::size_c<1>)
                );
        }
    }
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
