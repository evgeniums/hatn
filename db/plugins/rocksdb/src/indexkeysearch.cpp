/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/indexkeysearch.cpp
  *
  *   RocksDB index keys search.
  *
  */

/****************************************************************************/

#if defined(__MINGW32__) && defined(BUILD_DEBUG)
#define _GLIBCXX_DEBUG
#endif

#include <vector>

#include "rocksdb/comparator.h"

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/pmr/withstaticallocator.ipp>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/topic.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>
#include <hatn/db/query.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>
#include <hatn/db/plugins/rocksdb/modeltopics.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>

#include <hatn/db/plugins/rocksdb/indexkeysearch.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace index_key_search
{

IndexKey::IndexKey():partition(nullptr)
{}

IndexKey::IndexKey(
        ROCKSDB_NAMESPACE::Slice* k,
        ROCKSDB_NAMESPACE::Slice* v,
        const lib::string_view& topic,
        RocksdbPartition* p,
        bool unique
    ) : key(k->data(),k->size()),
        value(v->data(),v->size()),
        partition(p),
        topicLength(topic.size())
{
    keyParts.reserve(KeyPartsMax);
    fillKeyParts(topic,unique);
}

void IndexKey::fillKeyParts(const lib::string_view& topic, bool unique)
{
    size_t size=unique ? key.size() : (key.size()-ObjectId::Length);
    size_t offset=FieldsOffset+topic.size();
    for (size_t i=offset;i<size;i++)
    {
        if (key[i]==SeparatorCharC)
        {
            auto* k=key.data();
            auto* ptr=k+offset;
            auto count=i-offset;
            keyParts.emplace_back(ptr,count);
            offset=i;
        }
    }
}

lib::string_view IndexKey::keyPrefix(const lib::string_view& key, const lib::string_view& topic, size_t pos) noexcept
{
    size_t idx=0;

    for (size_t i=FieldsOffset+topic.size();i<key.size();i++)
    {
        if (key[i]==SeparatorCharC)
        {
            if (idx==(pos-1))
            {
                return lib::string_view{key.data(),i};
            }
            idx++;
        }
    }

    return lib::string_view{};
}

static bool filterIndex(const ModelIndexQuery& idxQuery,
                        size_t pos,
                        const ROCKSDB_NAMESPACE::Slice& key,
                        const ROCKSDB_NAMESPACE::Slice& value
                        )
{
    std::ignore=key;

    // filter by timestamp
    if (pos==1)
    {
        auto timestamp=Keys::timestampFromIndexValue(value.data(),value.size());
        return idxQuery.query.filterTimePoint(timestamp);
    }

    // passed
    return false;
}

Error iterateFieldVariant(
    Cursor& cursor,
    RocksdbHandler& handler,
    const ModelIndexQuery& idxQuery,
    const KeyHandlerFn& keyCallback,
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    const query::Field& field,
    AllocatorFactory* allocatorFactory,
    const Slice& prevFrom,
    const Slice& prevTo
    )
{
    size_t pos=cursor.pos;
    bool lastField=pos==idxQuery.query.fields().size();

    KeyBuf fromBuf;
    KeyBuf toBuf;
    Error ec;
    bool lastKey=false;
    bool seekExactPrefix=false;
    bool onlyFirst=false;
    bool onlyLast=false;
    ROCKSDB_NAMESPACE::Slice fromS;
    ROCKSDB_NAMESPACE::Slice toS;

    // prepare read options
    ROCKSDB_NAMESPACE::ReadOptions readOptions=handler.p()->readOptions;
    readOptions.snapshot=snapshot;
    bool iterateForward=field.order==query::Order::Asc;
    switch(field.op)
    {
        case (query::Operator::gt):
        {
            fromBuf.append(cursor.keyPrefix);
            fromBuf.append(SeparatorCharStr);
            fieldValueToBuf(fromBuf,field,SeparatorCharPlusStr);
            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
            readOptions.iterate_lower_bound=&fromS;
            toS=prevTo;
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::gte):
        {
            if (field.value.isFirst())
            {
                fromS=prevFrom;
                readOptions.iterate_lower_bound=&fromS;
            }
            else
            {
                fromBuf.append(cursor.keyPrefix);
                fromBuf.append(SeparatorCharStr);
                fieldValueToBuf(fromBuf,field,SeparatorCharStr);
                fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
                readOptions.iterate_lower_bound=&fromS;
            }
            toS=prevTo;
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::lt):
        {
            fromS=prevFrom;
            readOptions.iterate_lower_bound=&fromS;
            toBuf.append(cursor.keyPrefix);
            toBuf.append(SeparatorCharStr);
            fieldValueToBuf(toBuf,field,SeparatorCharStr);
            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
            readOptions.iterate_upper_bound=&toS;
        }
        break;
        case (query::Operator::lte):
        {
            fromS=prevFrom;
            readOptions.iterate_lower_bound=&fromS;

            if (field.value.isLast())
            {
                toS=prevTo;
                readOptions.iterate_upper_bound=&toS;
            }
            else
            {
                toBuf.append(cursor.keyPrefix);
                toBuf.append(SeparatorCharStr);
                fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
                toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
                readOptions.iterate_upper_bound=&toS;
            }
        }
        break;
        case (query::Operator::eq):
        {
            if (
                field.value.isFirst()
                ||
                field.value.isLast()
                )
            {
                fromS=prevFrom;
                readOptions.iterate_lower_bound=&fromS;
                toS=prevTo;
                readOptions.iterate_upper_bound=&toS;
                iterateForward=field.value.isFirst();
            }
            else
            {
                fromBuf.append(cursor.keyPrefix);
                fromBuf.append(SeparatorCharStr);
                fieldValueToBuf(fromBuf,field,SeparatorCharStr);
                fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
                readOptions.iterate_lower_bound=&fromS;

                toBuf.append(cursor.keyPrefix);
                toBuf.append(SeparatorCharStr);
                fieldValueToBuf(toBuf,field,SeparatorCharPlusStr);
                toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
                readOptions.iterate_upper_bound=&toS;
            }

            seekExactPrefix=true;
        }
        break;
        case (query::Operator::in):
        {
            fromBuf.append(cursor.keyPrefix);
            fromBuf.append(SeparatorCharStr);

            onlyFirst=field.value.fromIntervalType()==query::IntervalType::First
                        &&
                        field.value.toIntervalType()==query::IntervalType::First;

            onlyLast=field.value.fromIntervalType()==query::IntervalType::Last
                       &&
                       field.value.toIntervalType()==query::IntervalType::Last;

            if (onlyFirst || onlyLast)
            {
                fromS=prevFrom;
                readOptions.iterate_lower_bound=&fromS;
                toS=prevTo;
                readOptions.iterate_upper_bound=&toS;
                iterateForward=onlyFirst;
                seekExactPrefix=true;
            }
            else
            {
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

                toBuf.append(cursor.keyPrefix);
                toBuf.append(SeparatorCharStr);
                constexpr static const fieldValueToBufT<hana::false_> toValueToBuf{};
                if (field.value.toIntervalType()==query::IntervalType::Open
                    ||
                    field.value.toIntervalType()==query::IntervalType::Last
                    )
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
        }
        break;

        default:
        {
            Assert(false,"Invalid operand");
        }
        break;
    }

    auto offset=cursor.keyPrefix.size();
    while (!lastKey)
    {
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
        lastKey=!it->Valid();
        while (!lastKey)
        {
            auto key=it->key();
            auto keyValue=it->value();

            // check if key must be filtered
            //! @todo optimization: Check if filter would drop all the next keys and, thus, break iteration immediately
            bool keyFiltered=TtlMark::isExpired(keyValue) || filterIndex(idxQuery,pos,key,keyValue);
            if (!keyFiltered)
            {
                // construct key prefix
                auto currentKeyPrefix=IndexKey::keyPrefix(lib::toStringView(key),cursor.topic,pos);
                cursor.appendPrefix(currentKeyPrefix);

                // call result callback for finally assembled key
                if (lastField)
                {
                    if (!keyCallback(cursor.partition,cursor.topic,&key,&keyValue,ec))
                    {
                        return ec;
                    }

                    // if exact prefix then no more iteration needed
                    lastKey=seekExactPrefix && (onlyFirst || onlyLast || field.value.isLast() || field.value.isFirst());
                }
                else
                {
                    // process next field
                    ec=nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory,fromS,toS);

                    // if exact prefix then no more iteration needed
                    lastKey=seekExactPrefix;
                }
                HATN_CHECK_EC(ec)
            }

            // continue iteration
            if (!lastKey)
            {
                // get the next/prev key
                if (iterateForward)
                {
                    it->Next();
                }
                else
                {
                    it->Prev();
                }
                lastKey=!it->Valid();

                // if this is not a final key field (i.e., lastField) then avoid iterating over repeated keys and go to seek for the next key prefix
                if (!lastKey && !lastField && !keyFiltered)
                {
                    // check if key prefix is repeated at least twice
                    auto nextKeyPrefix=IndexKey::keyPrefix(lib::toStringView(it->key()),cursor.topic,pos);
                    if (nextKeyPrefix==std::string_view{cursor.keyPrefix})
                    {
                        // next key prefix is the same as current key prefix, i.e. the prefix repeats at least twice
                        // thus, invoke a new seek excluding current key prefix
                        if (iterateForward)
                        {
                            fromBuf.clear();
                            fromBuf.append(cursor.keyPrefix);
                            fromBuf.append(SeparatorCharPlusStr);
                            fromS=ROCKSDB_NAMESPACE::Slice{fromBuf.data(),fromBuf.size()};
                            readOptions.iterate_lower_bound=&fromS;
                        }
                        else
                        {
                            toBuf.clear();
                            toBuf.append(cursor.keyPrefix);
                            toBuf.append(SeparatorCharStr);
                            toS=ROCKSDB_NAMESPACE::Slice{toBuf.data(),toBuf.size()};
                            readOptions.iterate_upper_bound=&toS;
                        }

                        // restore key prefix in cursor
                        if (!keyFiltered)
                        {
                            cursor.restorePrefix(offset);
                        }

                        // break iteration in "while (it->Valid())" and go to next seek in "while (!lastKey)"
                        break;
                    }
                }
            }

            // restore key prefix in cursor
            if (!keyFiltered)
            {
                cursor.restorePrefix(offset);
            }
        }

        // check iterator status
        if (!it->status().ok())
        {
            HATN_CTX_SCOPE_ERROR("index-find")
            return makeError(DbError::READ_FAILED,it->status());
        }
    }

    // done
    return OK;
}

Error HATN_ROCKSDB_SCHEMA_EXPORT nextKeyField(
    Cursor& cursor,
    RocksdbHandler& handler,
    const ModelIndexQuery& idxQuery,
    const KeyHandlerFn& keyCallback,
    const ROCKSDB_NAMESPACE::Snapshot* snapshot,
    AllocatorFactory* allocatorFactory,
    const Slice& prevFrom,
    const Slice& prevTo
    )
{
    Assert(cursor.pos<idxQuery.query.fields().size(),"Out of range of cursor position of index query");

    // extract query field
    const auto& queryField=idxQuery.query.field(cursor.pos);
    ++cursor.pos;

    // glue scalar fields if operators and orders match
    if (cursor.pos<idxQuery.query.fields().size() && idxQuery.query.field(cursor.pos).matchScalarOp(queryField))
    {
        // append field to cursor
        cursor.keyPrefix.append(SeparatorCharStr);
        fieldValueToBuf(cursor.keyPrefix,queryField);

        // go to next field
        return nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory,prevFrom,prevTo);
    }

    // for neq operation split to lt and gt queries
    auto doNeq=[&](const auto& val)
    {
        query::Field fieldLt{queryField.fieldInfo,query::Operator::lt,val,queryField.order};
        auto ec=iterateFieldVariant(cursor,
                                      handler,
                                      idxQuery,
                                      keyCallback,
                                      snapshot,
                                      fieldLt,
                                      allocatorFactory
                                      ,prevFrom,prevTo
                                      );
        HATN_CHECK_EC(ec)

        query::Field fieldGt{queryField.fieldInfo,query::Operator::gt,val,queryField.order};
        ec=iterateFieldVariant(cursor,
                                 handler,
                                 idxQuery,
                                 keyCallback,
                                 snapshot,
                                 fieldGt,
                                 allocatorFactory
                                 ,prevFrom,prevTo
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
                                          handler,
                                          idxQuery,
                                          keyCallback,
                                          snapshot,
                                          queryField,
                                          allocatorFactory
                                          ,prevFrom,prevTo
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
        if (queryField.op==query::Operator::in)
        {
            // in
            auto ec=iterateFieldVariant(cursor,
                                          handler,
                                          idxQuery,
                                          keyCallback,
                                          snapshot,
                                          queryField,
                                          allocatorFactory
                                          ,prevFrom,prevTo
                                          );
            HATN_CHECK_EC(ec)
        }
        else
        {
            // nin
            auto doNinInterval=[&](const auto& val)
            {                
                if (val.from.type!=query::IntervalType::First)
                {
                    const auto &beforeVal=val.from.value;
                    query::Operator beforeOp=val.from.type==query::IntervalType::Open ? query::Operator::lte : query::Operator::lt;
                    query::Field fieldBefore{queryField.fieldInfo,beforeOp,beforeVal,queryField.order};
                    auto ec=iterateFieldVariant(cursor,
                                                  handler,
                                                  idxQuery,
                                                  keyCallback,
                                                  snapshot,
                                                  fieldBefore,
                                                  allocatorFactory
                                                  ,prevFrom,prevTo
                                                  );
                    HATN_CHECK_EC(ec)
                }

                if (val.to.type!=query::IntervalType::Last)
                {
                    const auto &afterVal=val.to.value;
                    query::Operator afterOp=val.to.type==query::IntervalType::Open ? query::Operator::gte : query::Operator::gt;
                    query::Field fieldAfter{queryField.fieldInfo,afterOp,afterVal,queryField.order};
                    auto ec=iterateFieldVariant(cursor,
                                             handler,
                                             idxQuery,
                                             keyCallback,
                                             snapshot,
                                             fieldAfter,
                                             allocatorFactory
                                            ,prevFrom,prevTo
                                             );
                    HATN_CHECK_EC(ec)
                }

                return Error{};
            };

            auto ec=queryField.value.handleInterval(doNinInterval);
            HATN_CHECK_EC(ec)
        }
    }
    else if (queryField.value.isVectorType())
    {
        // sort vector
        auto sortVector=[&queryField](const auto& vec)
        {
            using vectorType=std::decay_t<decltype(vec)>;
            using type=typename vectorType::value_type;
            static const std::less<type> less{};
            auto& v=const_cast<vectorType&>(vec);
            std::sort(
                    v.begin(),
                    v.end(),
                    [&queryField](const type& l, const type& r)
                    {                        
                        if (queryField.order==query::Order::Desc)
                        {
                            return !less(l,r);
                        }
                        return less(l,r);
                    }
                );
            return Error{};
        };
        std::ignore=queryField.value.handleVector(sortVector);

        // split to queries for each vector item
        if (queryField.op==query::Operator::in)
        {
            // in vector
            auto doEq=[&](const auto& val)
            {
                query::Field field{queryField.fieldInfo,query::Operator::eq,val,queryField.order};
                return iterateFieldVariant(cursor,
                                           handler,
                                           idxQuery,
                                           keyCallback,
                                           snapshot,
                                           field,
                                           allocatorFactory
                                           ,prevFrom,prevTo
                                           );
            };
            auto ec=queryField.value.eachVectorItem(doEq);
            HATN_CHECK_EC(ec)
        }
        else
        {
            auto doNinVector=[&](const auto& vec)
            {
                using valueTypeT=typename std::decay_t<decltype(vec)>::value_type;
                using valueType=typename query::ValueTypeTraits<valueTypeT>::type;

                if constexpr (!hana::is_a<query::IntervalTag,valueType>)
                {
                    for (size_t i=0;i<vec.size();i++)
                    {
                        const auto& currentValue=vec[i];
                        if (i==0)
                        {
                            // before first interval use lt
                            query::Field fieldBefore{queryField.fieldInfo,query::lt,currentValue,queryField.order};
                            auto ec=iterateFieldVariant(cursor,
                                                          handler,
                                                          idxQuery,
                                                          keyCallback,
                                                          snapshot,
                                                          fieldBefore,
                                                          allocatorFactory
                                                          ,prevFrom,prevTo
                                                          );
                            HATN_CHECK_EC(ec)
                        }
                        else
                        {
                            // use in for intermediate fields
                            const auto& prevValue=vec[i-1];
                            query::Interval<valueType> tmpInterval{
                                prevValue,
                                query::IntervalType::Open,
                                currentValue,
                                query::IntervalType::Open
                            };
                            query::Field field{queryField.fieldInfo,query::in,tmpInterval,queryField.order};
                            auto ec=iterateFieldVariant(cursor,
                                                          handler,
                                                          idxQuery,
                                                          keyCallback,
                                                          snapshot,
                                                          field,
                                                          allocatorFactory
                                                          ,prevFrom,prevTo
                                                          );
                            HATN_CHECK_EC(ec)
                        }

                        // after last value
                        if (i==(vec.size()-1))
                        {
                            // use gt
                            query::Field fieldAfter{queryField.fieldInfo,query::Operator::gt,currentValue,queryField.order};
                            auto ec=iterateFieldVariant(cursor,
                                                          handler,
                                                          idxQuery,
                                                          keyCallback,
                                                          snapshot,
                                                          fieldAfter,
                                                          allocatorFactory
                                                          ,prevFrom,prevTo
                                                          );
                            HATN_CHECK_EC(ec)
                        }
                    }
                }
                return Error{};
            };

            // inverse intervals and invoke in or lt/lte for first and gt/gte for last intervals
            auto ec=queryField.value.handleVector(doNinVector);
            HATN_CHECK_EC(ec)
        }
    }
    else if (queryField.value.isVectorIntervalType())
    {
        // sort/merge vector of intervals
        auto sortMergeVector=[&queryField](const auto& vecWrapper)
        {
            using wrapperType=std::decay_t<decltype(vecWrapper)>;

            auto& v=const_cast<wrapperType&>(vecWrapper);
            query::sortAndMerge(v.value,queryField.order);

            return Error{};
        };
        std::ignore=queryField.value.handleVectorInterval(sortMergeVector);

        // handler for intermediate intervals
        auto doIn=[&](const auto& val)
        {
            query::Field fieldLt{queryField.fieldInfo,query::Operator::in,val,queryField.order};
            return iterateFieldVariant(cursor,
                                       handler,
                                       idxQuery,
                                       keyCallback,
                                       snapshot,
                                       fieldLt,
                                       allocatorFactory
                                       ,prevFrom,prevTo
                                       );
        };

        // split to queries for each vector item
        if (queryField.op==query::Operator::in)
        {
            // in
            auto ec=queryField.value.eachVectorIntervalItem(doIn);
            HATN_CHECK_EC(ec)
        }
        else
        {
            // nin
            auto doNin=[&](const auto& vecWrapper)
            {
                using wrapperType=std::decay_t<decltype(vecWrapper)>;
                using intervalType=typename wrapperType::interval_type;
                const auto& vec=vecWrapper.value;

                for (size_t i=0;i<vec.size();i++)
                {
                    const auto& interval=vec[i];

                    if (i==0)
                    {
                        // before first interval use lt/lte
                        query::Operator beforeOp=interval.from.type==query::IntervalType::Open ? query::Operator::lte : query::Operator::lt;
                        query::Field fieldBefore{queryField.fieldInfo,beforeOp,interval.from.value,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      handler,
                                                      idxQuery,
                                                      keyCallback,
                                                      snapshot,
                                                      fieldBefore,
                                                      allocatorFactory
                                                      ,prevFrom,prevTo
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
                        query::Field field{queryField.fieldInfo,query::Operator::in,tmpInterval,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      handler,
                                                      idxQuery,
                                                      keyCallback,
                                                      snapshot,
                                                      field,
                                                      allocatorFactory
                                                      ,prevFrom,prevTo
                                                      );
                        HATN_CHECK_EC(ec)
                    }

                    // after last interval
                    if (i==vec.size()-1)
                    {
                        // use lt/lte
                        query::Operator afterOp=interval.to.type==query::IntervalType::Open ? query::Operator::gte : query::Operator::gt;
                        query::Field fieldAfter{queryField.fieldInfo,afterOp,interval.to.value,queryField.order};
                        auto ec=iterateFieldVariant(cursor,
                                                      handler,
                                                      idxQuery,
                                                      keyCallback,
                                                      snapshot,
                                                      fieldAfter,
                                                      allocatorFactory
                                                      ,prevFrom,prevTo
                                                      );
                        HATN_CHECK_EC(ec)
                    }
                }
                return Error{};
            };

            // inverse intervals and invoke in or lt/lte for first and gt/gte for last intervals
            auto ec=queryField.value.handleVectorInterval(doNin);
            HATN_CHECK_EC(ec)
        }
    }

    // done
    --cursor.pos;
    return OK;
}

Result<IndexKeys> HATN_ROCKSDB_SCHEMA_EXPORT indexKeys(
        const ROCKSDB_NAMESPACE::Snapshot* snapshot,
        RocksdbHandler& handler,
        const std::string& modelId,
        const ModelIndexQuery& idxQuery,
        const Partitions& partitions,
        AllocatorFactory* allocatorFactory,
        bool single,
        bool withPartitionQuery
    )
{
    HATN_CTX_SCOPE("indexkeys")
    auto limit = single ? 1 : idxQuery.query.limit();
    size_t partitionLimit = idxQuery.query.offset() + limit;
    size_t partitionCount = 0;
    bool skipBeforeOffset=idxQuery.query.offset()!=0 && (partitions.size()*idxQuery.query.topics().size()==1);

    IndexKeys keys{IndexKeyCompare{idxQuery},allocatorFactory->dataAllocator<IndexKey>()};

    auto keyCallback=[&limit,&keys,&idxQuery,allocatorFactory,&single,
        &partitionCount,partitionLimit,&partitions,skipBeforeOffset
        ]
        (RocksdbPartition* partition,
         const lib::string_view& topic,
         ROCKSDB_NAMESPACE::Slice* key,
         ROCKSDB_NAMESPACE::Slice* keyValue,
         Error&
        )
    {

//! @maybe Log debug
#if 0
        std::cout<<"Found key "<<logKey(*key)<<std::endl;
#endif

        // skip indexes below offset in case of one partition and topic
        if (skipBeforeOffset)
        {
            if (partitionCount<idxQuery.query.offset())
            {
                partitionCount++;
                return true;
            }
        }

        // insert found key
        auto it=keys.emplace(key,keyValue,topic,partition,idxQuery.query.index()->unique());
        if (idxQuery.query.offset()==0)
        {
            // cut keys number to limit
            if (limit!=0 && keys.size()>limit)
            {
                // if inserted key is over the limit then break current iteration because keys are pre-sorted
                // and all next keys will be dropped anyway
                auto last=--keys.end();
                if (it.first==last)
                {
                    keys.erase(last);
                    return false;
                }
                keys.erase(last);
            }
        }
        else
        {
            // collect offset+limit for each partition*topic to truncate from the head up to the offset later
            partitionCount++;
            if (partitionCount==partitionLimit)
            {
                return false;
            }
        }

        return true;
    };

    auto eachTopic=[&](const Topic& topic, const std::shared_ptr<RocksdbPartition>& partition)
    {
        HATN_CTX_SCOPE_PUSH("topic",topic.topic())
        HATN_CTX_SCOPE_PUSH("index",idxQuery.query.index()->name())

        partitionCount=0;

        Cursor cursor(idxQuery.modelIndexId,topic,partition.get());
        auto ec=nextKeyField(cursor,handler,idxQuery,keyCallback,snapshot,allocatorFactory,
                               cursor.indexRangeFromSlice(),
                               cursor.indexRangeToSlice()
                               );
        HATN_CHECK_EC(ec)

        HATN_CTX_SCOPE_POP()
        HATN_CTX_SCOPE_POP()
        return Error{OK};
    };

    // process all partitions
    for (const auto& partition: partitions)
    {
        if (!partition->range.isNull())
        {
            HATN_CTX_SCOPE_PUSH("partition",partition->range)

//! @maybe Log debug
#if 0
            std::cout<<"Looking in partition "<<partition->range.toString()<<std::endl;
#endif
        }

        // iterate topics
        if (idxQuery.query.topics().empty())
        {
            auto topics=ModelTopics::modelTopics(modelId,handler,partition.get());
            HATN_CHECK_RESULT(topics)
            for (const auto& topic: topics.value())
            {
                auto ec=eachTopic(topic,partition);
                HATN_CHECK_EC(ec)
            }
        }
        else
        {
            for (const auto& topic: idxQuery.query.topics())
            {
                auto ec=eachTopic(topic,partition);
                HATN_CHECK_EC(ec)
            }
        }

        if (!partition->range.isNull())
        {
            HATN_CTX_SCOPE_POP()
        }

        // if query includes partition query then partitions are altready presorted by that field
        if (withPartitionQuery && keys.size()>=partitionLimit)
        {
            // break iteration if limit reached
            break;
        }
    }

    // if offset is not zero then cut the keys
    if (idxQuery.query.offset()!=0)
    {
        // remove from the beginning only if there are >1 partitions
        // because in case of only 1 partition the keys are already skipped
        if (!skipBeforeOffset)
        {
            if (idxQuery.query.offset()>=keys.size())
            {
                keys.clear();
            }
            else
            {
                for (size_t i=0;i<idxQuery.query.offset();i++)
                {
                    keys.erase(keys.begin());
                }
            }
        }
        while (keys.size()>limit)
        {
            keys.erase(--keys.end());
        }
    }

    // done
    return keys;
}

} // namespace index_key_search

HATN_ROCKSDB_NAMESPACE_END
