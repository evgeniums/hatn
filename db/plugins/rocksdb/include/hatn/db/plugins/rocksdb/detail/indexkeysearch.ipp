/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/indexkeysearch.ipp
  *
  *   RocksDB index keys search.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBINDEXKEYSEARCH_IPP
#define HATNROCKSDBINDEXKEYSEARCH_IPP

#include <cstddef>

#include "rocksdb/comparator.h"

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace index_key_search
{

struct IndexKey
{
    constexpr static const size_t FieldsOffset=ObjectId::Length+2*sizeof(SeparatorCharC)+common::Crc32HexLength;

    IndexKey(
        ROCKSDB_NAMESPACE::Slice* k,
        ROCKSDB_NAMESPACE::Slice* v,
        RocksdbPartition* p,
        AllocatorFactory* allocatorFactory
        ) : key(k->data(),k->size(),allocatorFactory->bytesAllocator()),
        value(v->data(),v->size(),allocatorFactory->bytesAllocator()),
        partition(p),
        keyParts(allocatorFactory->dataAllocator<lib::string_view>())
    {
        fillKeyParts();
    }

    inline void fillKeyParts()
    {
        // split key to key parts
        size_t offset=FieldsOffset;
        for (size_t i=FieldsOffset;i<key.size();i++)
        {
            if (key[i]==SeparatorCharC)
            {
                keyParts.push_back(ROCKSDB_NAMESPACE::Slice{key.data()+offset,i-offset});
                offset=i;
            }
        }
    }

    static lib::string_view keyPrefix(const lib::string_view& key, size_t pos) noexcept
    {
        size_t idx=0;

        for (size_t i=FieldsOffset;i<key.size();i++)
        {
            if (key[i]==SeparatorCharC)
            {
                if (idx==pos)
                {
                    return lib::string_view{key.data(),i};
                }
                idx++;
            }
        }

        return lib::string_view{};
    }

    common::pmr::string key;
    common::pmr::string value;
    RocksdbPartition* partition;

    common::pmr::vector<ROCKSDB_NAMESPACE::Slice> keyParts;
};

struct IndexKeyCompare
{
    IndexKeyCompare(const IndexQuery& idxQuery) : idxQuery(&idxQuery)
    {}

    inline operator bool()(const IndexKey& l, const IndexKey& r) const noexcept
    {
        // compare key parts according to ordering of query fields
        for (size_t i=0;i<idxQuery->fields().size();i++)
        {
            if (i>=l.keyParts.size() || i>=r.keyParts.size())
            {
                return false;
            }

            const auto& field=idxQuery->field(i);
            const auto& leftPart=l.keyParts[i];
            const auto& rightPart=r.keyParts[i];
            int cmp{0};
            if (field.order==query::Order::Desc)
            {
                cmp=rightPart.compare(leftPart);
            }
            else
            {
                cmp=leftPart.compare(rightPart)<0;
            }
            if (cmp!=0)
            {
                return cmp<0;
            }
        }

        return false;
    }

    const IndexQuery* idxQuery;
};

using IndexKeys=common::pmr::FlatSet<IndexKey,IndexKeyCompare>;

template <typename BufT>
struct Cursor
{
    Cursor(
            const lib::string_view& indexId,
            const lib::string_view& topic,
            RocksdbPartition* partition,
            common::pmr::AllocatorFactory* allocatorfactory
        ) : partialKey(allocatorfactory->bytesAllocator()),
            pos(0),
            partition(partition)
    {
        partialKey.append(topic);
        partialKey.append(SeparatorCharStr);
        partialKey.append(indexId);
    }

    void resetPartial(const lib::string_view& prefixKey, size_t p)
    {
        partialKey.clear();
        partialKey.append(prefixKey);
        pos=p;
    }

    BufT partialKey;
    size_t pos;

    RocksdbPartition* partition;
};

template <typename BufT, typename EndpointT=hana::true_>
struct valueVisitor
{
    constexpr static const EndpointT endpoint{};

    template <typename T>
    void operator()(const query::Interval<T>& value) const
    {
        hana::if_(
            endpoint,
            fieldToStringBuf(buf,value.from),
            fieldToStringBuf(buf,value.to)
            );
        buf.append(sep);
    }

    template <typename T>
    void operator()(const T& value) const
    {
        fieldToStringBuf(buf,value);
        buf.append(sep);
    }

    void operator()(const query::Last&) const
    {
        // replace previous separator with SeparatorCharPlusStr
        buf[buf.size()-1]=SeparatorCharPlusStr[0];
    }

    void operator()(const query::First&) const
    {}

    valueVisitor(BufT& buf, const lib::string_view& sep):buf(buf),sep(sep)
    {}

    BufT& buf;
    const lib::string_view& sep;
};

template <typename EndpointT=hana::true_>
struct fieldValueToBufT
{
    template <typename BufT>
    void operator()(BufT& buf, const query::Field& field, const lib::string_view& sep) const
    {
        lib::variantVisit(valueVisitor<BufT,EndpointT>{buf,sep},field.value());
    }
};
constexpr fieldValueToBufT<hana::true_> fieldValueToBuf{};

inline bool filterIndex(const IndexQuery& idxQuery, size_t pos,
                        const ROCKSDB_NAMESPACE::Slice* key,
                        const ROCKSDB_NAMESPACE::Slice* value)
{
    //! @todo Implement index filtering
    return false;
}

template <typename BufT, typename KeyHandlerT>
Error iterateFieldVariant(
    Cursor<BufT>& cursor,
    RocksdbHandler& handler,
    const IndexQuery& idxQuery,
    const KeyHandlerT& keyCallback,
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    const query::Field& field,
    AllocatorFactory* allocatorFactory
    )
{
    size_t pos=cursor.pos;

    // prepare read options
    ROCKSDB_NAMESPACE::ReadOptions readOptions=handler.p()->readOptions;
    readOptions.snapshot=snapshot;
    bool iterateForward=field.order==query::Order::Asc;

    BufT fromBuf{allocatorFactory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice fromS;
    BufT toBuf{allocatorFactory->bytesAllocator()};
    ROCKSDB_NAMESPACE::Slice toS;
    switch(field.op)
    {
        case (query::Operator::gt):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharPlusStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::gte):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
        }
        break;
        case (query::Operator::lt):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::lte):
        {
            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::eq):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::in):
        {
            fromBuf.append(cursor.partialKey);
            fromBuf.append(SeparatorCharStr);
            if (field.value.fromIntervalType()==query::IntervalType::Open)
            {
                fieldValueToBuf(fromBuf,field,SeparatorCharPlusStr);
            }
            else
            {
                fieldValueToBuf(fromBuf,field,SeparatorCharStr);
            }
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;

            toBuf.append(cursor.partialKey);
            toBuf.append(SeparatorCharStr);
            constexpr static const fieldValueToBufT<hana::false_> toValueToBuf{};
            if (field.value.toIntervalType()==query::IntervalType::Open)
            {
                toValueToBuf(toBuf,field,SeparatorCharStr);
            }
            else
            {
                toValueToBuf(toBuf,field,SeparatorCharPlusStr);
            }
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;

        default:
        {
            Assert(false,"Invalid operand");
        }
        break;
    }

    // create iterator
    std::unique_ptr<ROCKSDB_NAMESPACE::Iterator> it{handler.p()->db->NewIterator(readOptions,cursor.partition->indexCf.get())};

    // set start position of the iterator
    if (iterateForward)
    {
        it->SeekToFirst();
    }
    else
    {
        it->SeekToLast();
    }

    // iterate
    Error ec;
    while (it->Valid())
    {
        const auto* key=it->Key();
        const auto* keyValue=it->Slice();

        // check if key must be filtered
        if (!TtlMark::isExpired(keyValue)
            && !filterIndex(idxQuery,pos,key,keyValue)
            )
        {
            // construct key prefix
            auto currentKey=IndexKey::keyPrefix(key,pos);
            cursor.resetPartial(currentKey,pos);

            // call result callback for finally assembled key
            if (pos==idxQuery.fields().size())
            {
                if (!keyCallback(cursor.partition,key,keyValue,ec))
                {
                    return ec;
                }
            }
            else
            {
                auto ec=nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
                HATN_CHECK_EC(ec)
            }
        }

        // get the next/prev key
        if (iterateForward)
        {
            it->Next();
        }
        else
        {
            it->Prev();
        }
    }
    // check iterator status
    if (!it->status().ok())
    {
        HATN_CTX_SCOPE_ERROR("index-find")
        return makeError(DbError::READ_FAILED,it->status());
    }

    // done
    return OK;
}

template <typename BufT, typename KeyHandlerT>
Error nextKeyField(
    Cursor<BufT>& cursor,
    RocksdbHandler& handler,
    const IndexQuery& idxQuery,
    const KeyHandlerT& keyCallback,
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    AllocatorFactory* allocatorFactory
    )
{
    Assert(cursor.pos<idxQuery.fields().size(),"Out of range of cursor position of index query");

    // extract query field
    const auto& queryField=idxQuery.field(cursor.pos);
    ++cursor.pos;

    // glue scalar fields if operators and orders match
    if (cursor.pos<idxQuery.fields().size() && idxQuery.field(cursor.pos).matchScalarOp(queryField))
    {
        // append field to cursor
        cursor.partialKey.append(SeparatorCharStr);
        fieldValueToBuf(cursor.partialKey,queryField.value());

        // go to next field
        return nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory);
    }

    // for neq operation split to lt and gt queries
    auto doNeq=[&](const auto& val)
    {
        query::Field fieldLt{*queryField.fieldInfo,query::Operator::lt,val,queryField.order};
        auto ec=iterateFieldVariant(cursor,
                                      idxQuery,
                                      handler,
                                      keyCallback,
                                      snapshot,
                                      fieldLt,
                                      allocatorFactory
                                      );
        HATN_CHECK_EC(ec)

        query::Field fieldGt{*queryField.fieldInfo,query::Operator::gt,val,queryField.order};
        ec=iterateFieldVariant(cursor,
                                 idxQuery,
                                 handler,
                                 keyCallback,
                                 snapshot,
                                 fieldGt,
                                 allocatorFactory
                                 );
        HATN_CHECK_EC(ec)

        return Error{};
    };

    // iterate
    if (queryField.value.isScalarType())
    {
        // process scalar fields
        if (queryField.op!=query::Operator::neq)
        {
            auto ec=iterateFieldVariant(cursor,
                                          idxQuery,
                                          handler,
                                          keyCallback,
                                          snapshot,
                                          queryField,
                                          allocatorFactory
                                          );
            HATN_CHECK_EC(ec)
        }
        else
        {
            // neq
            auto ec=doNeq(queryField.value);
            HATN_CHECK_EC(ec)
        }
    }
    else if (queryField.value.isIntervalType())
    {
        // process interval fields
        if (queryField.op!=query::Operator::in)
        {
            // in
            auto ec=iterateFieldVariant(cursor,
                                          idxQuery,
                                          handler,
                                          keyCallback,
                                          snapshot,
                                          queryField,
                                          allocatorFactory
                                          );
            HATN_CHECK_EC(ec)
        }
        else
        {
            // nin
            auto doNin=[&](const auto& val)
            {
                const auto &beforeVal=val.from.value;
                query::Operator beforeOp=val.from.type==query::IntervalType::Open ? query::Operator::lte : query::Operator::lt;

                query::Field fieldBefore{*queryField.fieldInfo,beforeOp,beforeVal,queryField.order};
                auto ec=iterateFieldVariant(cursor,
                                              idxQuery,
                                              handler,
                                              keyCallback,
                                              snapshot,
                                              fieldBefore,
                                              allocatorFactory
                                              );
                HATN_CHECK_EC(ec)

                const auto &afterVal=val.to.value;
                query::Operator afterOp=val.to.type==query::IntervalType::Open ? query::Operator::gte : query::Operator::gt;

                query::Field fieldAfter{*queryField.fieldInfo,afterOp,afterVal,queryField.order};
                ec=iterateFieldVariant(cursor,
                                         idxQuery,
                                         handler,
                                         keyCallback,
                                         snapshot,
                                         fieldAfter,
                                         allocatorFactory
                                         );
                HATN_CHECK_EC(ec)

                return Error{};
            };
            auto ec=queryField.value.handleInterval(doNin);
            HATN_CHECK_EC(ec)
        }
    }
    else if (queryField.value.isVectorType())
    {
        // sort vector
        auto sortVector=[&queryField](const auto& vec)
        {
            using vectorType=std::decay_t<decltype(vec)>;
            auto& v=const_cast<vectorType&>(vec);
            std::sort(
                    v.begin(),
                    v.end(),
                    [&queryField](const auto& l, const auto& r)
                    {
                        using type=std::decay_t<decltype(l)>;
                        if (queryField.order==query::Order::Desc)
                        {
                            return !std::less<type>{}(l,r);
                        }
                        return std::less<type>{}(l,r);
                    }
                );
        };
        queryField.value.handleVector(sortVector);

        // split to queries for each vector item
        if (queryField.op==query::Operator::in)
        {
            // in
            auto doEq=[&](const auto& val)
            {
                query::Field field{*queryField.fieldInfo,query::Operator::eq,val,queryField.order};
                return iterateFieldVariant(cursor,
                                           idxQuery,
                                           handler,
                                           keyCallback,
                                           snapshot,
                                           field,
                                           allocatorFactory
                                           );
            };
            auto ec=queryField.value.eachVectorItem(doEq);
            HATN_CHECK_EC(ec)
        }
        else
        {
            // nin
            auto ec=queryField.value.eachVectorItem(doNeq);
            HATN_CHECK_EC(ec)
        }
    }
    else if (queryField.value.isVectorIntervalType())
    {
        // sort vector
        auto sortVector=[&queryField](const auto& vec)
        {
            using vectorType=std::decay_t<decltype(vec)>;
            auto& v=const_cast<vectorType&>(vec);
            std::sort(
                v.begin(),
                v.end(),
                [&queryField](const auto& l, const auto& r)
                {
                    using type=std::decay_t<decltype(l)>;
                    if (queryField.order==query::Order::Desc)
                    {
                        return !std::less<type>{}(l,r);
                    }
                    return std::less<type>{}(l,r);
                }
                );
        };
        queryField.value.handleVector(sortVector);

        // handler for intermediate intervals
        auto doIn=[&](const auto& val)
        {
            query::Field fieldLt{*queryField.fieldInfo,query::Operator::in,val,queryField.order};
            return iterateFieldVariant(cursor,
                                       idxQuery,
                                       handler,
                                       keyCallback,
                                       snapshot,
                                       fieldLt,
                                       allocatorFactory
                                       );
        };

        // split to queries for each vector item
        if (queryField.op==query::Operator::in)
        {
            // in
            auto ec=queryField.value.eachVectorItem(doIn);
            HATN_CHECK_EC(ec)
        }
        else
        {
            // nin
            auto doNin=[&](const auto& vec)
            {
                using intervalType=typename std::decay_t<decltype(vec)>::value_type;
                using valueType=typename intervalType::Valuetype;

                for (size_t i=0;i<vec.size();i++)
                {
                    const auto& interval=vec[i];

                    if (i==0)
                    {
                        // before first interval use lt/lte
                        query::Operator beforeOp=interval.from.type==query::IntervalType::Open ? query::Operator::lte : query::Operator::lt;
                        query::Field fieldBefore{*queryField.fieldInfo,beforeOp,interval.from.value,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      idxQuery,
                                                      handler,
                                                      keyCallback,
                                                      snapshot,
                                                      fieldBefore,
                                                      allocatorFactory
                                                      );
                        HATN_CHECK_EC(ec)
                    }
                    else
                    {
                        // use in for intermediate intervals
                        const auto& prevInterval=vec[i-1];
                        intervalType tmpInterval{
                            prevInterval.to.value,
                            query::reverseIntervalType(prevInterval.to.type),
                            interval.from.value,
                            query::reverseIntervalType(interval.from.type)
                        };
                        query::Field field{*queryField.fieldInfo,query::Operator::in,tmpInterval,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      idxQuery,
                                                      handler,
                                                      keyCallback,
                                                      snapshot,
                                                      field,
                                                      allocatorFactory
                                                      );
                        HATN_CHECK_EC(ec)
                    }

                    // after last interval
                    if (i==vec.size()-1)
                    {
                        // use lt/lte
                        query::Operator afterOp=interval.to.type==query::IntervalType::Open ? query::Operator::gte : query::Operator::gt;
                        query::Field fieldAfter{*queryField.fieldInfo,afterOp,interval.to.value,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      idxQuery,
                                                      handler,
                                                      keyCallback,
                                                      snapshot,
                                                      fieldAfter,
                                                      allocatorFactory
                                                      );
                        HATN_CHECK_EC(ec)
                    }
                }

                return Error{};
            };


            // inverse intervals and invoke in or lt/lte for first and gt/gte for last intervals
            auto ec=queryField.value.handleVector(doNin);
            HATN_CHECK_EC(ec)
        }
    }

    // done
    --cursor.pos;
    return OK;
}

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXKEYSEARCH_IPP
