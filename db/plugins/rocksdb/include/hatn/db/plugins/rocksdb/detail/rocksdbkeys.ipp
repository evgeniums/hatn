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
                        auto nextPos=hana::plus(_(pos),hana::size_c<1>);
                        for (size_t i=0;i<_(field).count();i++)
                        {
                            auto sizeBefore=_(buf).size();

                            const auto& val=_(field).at(i);
                            if constexpr (std::is_base_of<du::detail::BytesTraitsBase,std::decay_t<decltype(val)>>::value)
                            {
                                fieldToStringBuf(_(buf),val.stringView());
                            }
                            else
                            {
                                fieldToStringBuf(_(buf),val);
                            }
                            _(buf).append(SeparatorCharStr);
                            auto ec=_(self)->iterateIndexFields(
                                _(buf),
                                _(objectId),
                                _(object),
                                _(index),
                                _(handler),
                                nextPos
                                );
                            HATN_CHECK_EC(ec)

                            _(buf).resize(sizeBefore);
                        }
                        return Error{OK};
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
                    // else Null index just append separator
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

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
