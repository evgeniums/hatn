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

#include <hatn/dataunit/fields/map.h>

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
        PosT pos,
        IsIndexSet isIndexSet
    )
{
    if constexpr (decltype(hana::equal(pos,hana::size(index.fields)))::value)
    {
        IndexKeySlice key;
        key[0]=ROCKSDB_NAMESPACE::Slice{buf.data(),buf.size()};
        key[1]=objectId;
        return handler(key,isIndexSet);
    }
    else
    {
        const auto& fieldId=hana::at(index.fields,pos);
        const auto* fieldPtr=getIndexFieldPtr(*object,fieldId);
        using type=typename std::pointer_traits<decltype(fieldPtr)>::element_type;

        using fieldT=type;
        using repeatedT=typename fieldT::isRepeatedType;
        using mapT=typename fieldT::isMapType;

        if constexpr (repeatedT::value)
        {
            if (fieldPtr!=nullptr && fieldPtr->isSet())
            {
                const auto& field=*fieldPtr;
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

                    if (isIndexSet==IsIndexSet::Unknown)
                    {
                        isIndexSet=IsIndexSet::Yes;
                    }

                    auto ec=iterateIndexFields(
                        buf,
                        objectId,
                        object,
                        index,
                        handler,
                        hana::plus(pos,hana::size_c<1>),
                        isIndexSet
                        );
                    HATN_CHECK_EC(ec)

                    buf.resize(sizeBefore);
                }
                return Error{OK};
            }
            else
            {
                isIndexSet=IsIndexSet::No;

                buf.append(SeparatorCharStr);
                return iterateIndexFields(
                    buf,
                    objectId,
                    object,
                    index,
                    handler,
                    hana::plus(pos,hana::size_c<1>),
                    isIndexSet
                );
            }
        }
        else if constexpr (mapT::value)
        {
            auto nextPos=hana::plus(pos,hana::size_c<1>);
            static_assert(decltype(hana::less(nextPos,hana::size(index.fields)))::value,"Index must include either map's key or map's value");

            if (fieldPtr!=nullptr && fieldPtr->isSet())
            {
                const auto& field=*fieldPtr;
                const auto& subfieldId=hana::at(index.fields,nextPos);
                using subfieldType=std::decay_t<decltype(subfieldId)>;

                for (const auto& it: field.value())
                {
                    auto sizeBefore=buf.size();

                    if (subfieldType::id==du::MapItemKey::id)
                    {
                        if (isIndexSet==IsIndexSet::Unknown)
                        {
                            isIndexSet=IsIndexSet::Yes;
                        }

                        const auto& key=it.first;
                        fieldToStringBuf(buf,key);
                        buf.append(SeparatorCharStr);
                        auto ec=iterateIndexFields(
                            buf,
                            objectId,
                            object,
                            index,
                            handler,
                            hana::plus(nextPos,hana::size_c<1>),
                            isIndexSet
                            );
                        HATN_CHECK_EC(ec)
                    }
                    else if (subfieldType::id==du::MapItemValue::id)
                    {
                        if (isIndexSet==IsIndexSet::Unknown)
                        {
                            isIndexSet=IsIndexSet::Yes;
                        }

                        const auto& value=it.second;
                        fieldToStringBuf(buf,value);
                        buf.append(SeparatorCharStr);
                        auto ec=iterateIndexFields(
                            buf,
                            objectId,
                            object,
                            index,
                            handler,
                            hana::plus(nextPos,hana::size_c<1>),
                            isIndexSet
                            );
                        HATN_CHECK_EC(ec)
                    }

                    buf.resize(sizeBefore);
                }
                return Error{OK};
            }
            else
            {
                isIndexSet=IsIndexSet::No;

                buf.append(SeparatorCharStr);
                return iterateIndexFields(
                    buf,
                    objectId,
                    object,
                    index,
                    handler,
                    hana::plus(pos,hana::size_c<1>),
                    isIndexSet
                );
            }
        }
        else
        {
            if (fieldPtr!=nullptr && (fieldPtr->fieldHasDefaultValue() || fieldPtr->isSet()))
            {
                const auto& field=*fieldPtr;
                if (isIndexSet==IsIndexSet::Unknown)
                {
                    isIndexSet=IsIndexSet::Yes;
                }
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
                hana::plus(pos,hana::size_c<1>),
                isIndexSet
            );
        }
    }
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
