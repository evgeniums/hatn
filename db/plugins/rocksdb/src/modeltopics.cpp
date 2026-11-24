/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/modeltopics.cpp
  *
  *   RocksDB model-topic relations.
  *
  */

/****************************************************************************/

#include <boost/endian/conversion.hpp>

#include <rocksdb/db.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/plugins/rocksdb/modeltopics.h>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbkeys.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>

#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

Error ModelTopics::update(
        const std::string& modelId,
        Topic topic,
        RocksdbHandler &handler,
        RocksdbPartition* partition,
        Operator action
    )
{
    std::array<char,MaxOperationSize> op;

    size_t size=0;
    op[0]=static_cast<char>(action);
    size+=sizeof(Operator);
    ROCKSDB_NAMESPACE::Slice ops(op.data(),size);

    KeyBuf key;
    fillRelationKey(modelId,topic,key);
    ROCKSDB_NAMESPACE::Slice ks(key.data(),key.size());

//! @maybe Log debug
#if 0
    std::cout << "MergeModelTopic::upate op=" << static_cast<int>(op[0]) << " key=" << logKey(key)
              << " size=" << size << std::endl;
#endif
    auto status=handler.p()->db->Merge(
            handler.p()->writeOptions,
            partition->dataCf(),
            ks,
            ops
        );
    if (!status.ok())
    {
        handler.p()->noteWriteStatus(status);
        return makeError(DbError::MODEL_TOPIC_RELATION_SAVE,status);
    }
    return OK;
}

//---------------------------------------------------------------

bool ModelTopics::deserializeOperation(const char *data, size_t size, Operation &op)
{
//! @maybe Log debug
#if 0
    std::cout << "ModelTopics::deserializeOperation size="<<size << std::endl;
#endif
    if (size!=sizeof(Operator))
    {
        return false;
    }

    op.op=static_cast<Operator>(data[0]);

    return true;
}

//---------------------------------------------------------------

void ModelTopics::serializeRelation(const Relation &rel, std::string *value)
{
    auto newSize=sizeof(rel.version)+sizeof(rel.count)+rel.ttl.size();
    value->resize(newSize);

    (*value)[0]=static_cast<char>(rel.version);

    uint64_t count=boost::endian::native_to_little(rel.count);
    memcpy(value->data()+sizeof(rel.version),&count,sizeof(rel.count));
    rel.ttl.copy(value->data()+sizeof(rel.version)+sizeof(rel.count),rel.ttl.size());

//! @maybe Log debug
#if 0
    std::cout << "ModelTopics::serializeRelation size="<<value->size() << " expected size="<<newSize  << std::endl;
#endif
}

//---------------------------------------------------------------

void ModelTopics::serializeDelta(const Delta &delta, std::string *value)
{
    value->resize(DeltaSize);

    (*value)[0]=static_cast<char>(DeltaMarker);

    uint64_t change=boost::endian::native_to_little(static_cast<uint64_t>(delta.change));
    uint64_t floor=boost::endian::native_to_little(delta.floor);
    memcpy(value->data()+sizeof(uint8_t),&change,sizeof(change));
    memcpy(value->data()+sizeof(uint8_t)+sizeof(change),&floor,sizeof(floor));
}

//---------------------------------------------------------------

bool ModelTopics::deserializeDelta(const char *data, size_t size, Delta &delta)
{
    if (size!=DeltaSize || static_cast<uint8_t>(data[0])!=DeltaMarker)
    {
        return false;
    }

    uint64_t change=0;
    uint64_t floor=0;
    memcpy(&change,data+sizeof(uint8_t),sizeof(change));
    memcpy(&floor,data+sizeof(uint8_t)+sizeof(change),sizeof(floor));
    delta.change=static_cast<int64_t>(boost::endian::little_to_native(change));
    delta.floor=boost::endian::little_to_native(floor);

    return true;
}

//---------------------------------------------------------------

bool ModelTopics::deserializeRelation(const char *data, size_t size, Relation &rel)
{

//! @maybe Log debug
#if 0
    std::cout << "ModelTopics::deserializeRelation size="<<size << std::endl;
#endif

    if (size!=MinRelationSize && size!=MaxRelationSize)
    {
        return false;
    }

    rel.version=data[0];
    uint64_t count=0;
    memcpy(&count,data+sizeof(rel.version),sizeof(rel.count));
    rel.count=boost::endian::little_to_native(count);
    rel.ttl.load(data+sizeof(rel.version)+sizeof(rel.count),size-sizeof(rel.version)-sizeof(rel.count));

    return true;
}

//---------------------------------------------------------------

void ModelTopics::fillRelationKey(const std::string& modelId, Topic topic, KeyBuf& key)
{
    key.clear();
    key.append(RelationKeyPrefix);
    key.append(SeparatorCharStr);
    key.append(modelId);
    key.append(SeparatorCharStr);
    key.append(topic.topic());
}

//---------------------------------------------------------------

void ModelTopics::fillModelKeyPrefix(const std::string& modelId, KeyBuf& key, bool to)
{
    key.clear();
    key.append(RelationKeyPrefix);
    key.append(SeparatorCharStr);
    key.append(modelId);
    if (to)
    {
        key.append(SeparatorCharPlusStr);
    }
    else
    {
        key.append(SeparatorCharStr);
    }
}

//---------------------------------------------------------------

Result<size_t> ModelTopics::count(
        const ModelInfo& model,
        Topic topic,
        const common::Date &date,
        RocksdbHandler &handler
    )
{
    // construct keys
    auto rdOpts=handler.p()->readOptions;
    KeyBuf key;
    KeyBuf keyTo;
    Slice ksTo;
    Slice ks;
    bool multipleTopics=topic.topic().empty();
    if (!multipleTopics)
    {
        fillRelationKey(model.modelIdStr(),topic,key);
        ks=Slice{key.data(),key.size()};
    }
    else
    {
        fillModelKeyPrefix(model.modelIdStr(),key);
        ks=Slice{key.data(),key.size()};
        fillModelKeyPrefix(model.modelIdStr(),keyTo,true);
        ksTo=Slice{keyTo.data(),keyTo.size()};
        rdOpts.iterate_lower_bound=&ks;
        rdOpts.iterate_upper_bound=&ksTo;
    }

    size_t count=0;
    auto eachTopic=[&count](const auto& readSl)
    {
        Relation rel;
        auto ok=deserializeRelation(readSl.data(),readSl.size(),rel);
        if (!ok)
        {
            return dbError(DbError::MODEL_TOPIC_RELATION_DESER);
        }

        count+=rel.count;

//! @maybe Log debug
#if 0
        std::cout << "ModelTopics eachTopic " << logKey(readSl)
                  << " rel.count " << rel.count
                  << " total.count " << count
                  << std::endl;
#endif

        return Error{OK};
    };

    auto eachPartition=[multipleTopics,&eachTopic,&handler,&ks,&rdOpts](std::shared_ptr<RocksdbPartition>& partition)
    {
        if (multipleTopics)
        {
//! @maybe Log debug
#if 0
            std::cout << "ModelTopics from " << logKey(ks)
                      << " to " << logKey(ksTo)
                      << std::endl;
#endif
            std::unique_ptr<ROCKSDB_NAMESPACE::Iterator> it{handler.p()->db->NewIterator(rdOpts,partition->dataCf())};
            it->SeekToFirst();
            auto hasKey=it->Valid();
            while (hasKey)
            {
                auto relSl=it->value();
                auto ec=eachTopic(relSl);
                HATN_CHECK_EC(ec)

                it->Next();
                hasKey=it->Valid();
            }
            if (!it->status().ok())
            {
                if (it->status().code()==ROCKSDB_NAMESPACE::Status::kNotFound)
                {
                    return Error{OK};
                }
                return makeError(DbError::MODEL_TOPIC_RELATION_READ,it->status());
            }
        }
        else
        {
//! @maybe Log debug
#if 0
            std::cout << "Single topic from " << logKey(ks) << std::endl;
#endif
            ROCKSDB_NAMESPACE::PinnableSlice readSl;
            auto status=handler.p()->db->Get(rdOpts,partition->dataCf(),ks,&readSl);
            if (!status.ok())
            {
                if (status.code()==ROCKSDB_NAMESPACE::Status::kNotFound)
                {
                    return Error{OK};
                }
                return makeError(DbError::MODEL_TOPIC_RELATION_READ,status);
            }
            auto ec=eachTopic(readSl);
            HATN_CHECK_EC(ec)
        }

        return Error{OK};
    };

    if (!date.isNull())
    {
        std::shared_ptr<RocksdbPartition> partition;
        auto range=datePartition(date,model);
        if (!range.isNull())
        {
            partition=handler.partition(range);
        }
        if (!partition)
        {
            return dbError(DbError::PARTITION_NOT_FOUND);
        }

        auto ec=eachPartition(partition);
        HATN_CHECK_EC(ec)
    }
    else
    {
        if (model.isDatePartitioned())
        {
            std::vector<std::shared_ptr<RocksdbPartition>> partitions;
            {
                common::lib::shared_lock<common::lib::shared_mutex> l{handler.p()->partitionMutex};
                partitions.reserve(handler.p()->partitions.size());
                for (auto& it: handler.p()->partitions)
                {
                    partitions.push_back(it);
                }
            }

            for (auto&& it: partitions)
            {
                auto ec=eachPartition(it);
                HATN_CHECK_EC(ec)
            }
        }
        else
        {
            auto ec=eachPartition(handler.p()->defaultPartition);
            HATN_CHECK_EC(ec)
        }
    }

    return count;
}

namespace {

#if 0
inline Topic topicFromKey(const Slice& key)
{
    Topic topic;
    if (key.size()>ModelTopics::KeyPrefixOffset)
    {
        return lib::string_view{key.data()+ModelTopics::KeyPrefixOffset,key.size()-ModelTopics::KeyPrefixOffset};
    }
    return topic;
}
#endif

inline lib::string_view topicFromKey(const Slice& key)
{
    if (key.size()>ModelTopics::KeyPrefixOffset)
    {
        return lib::string_view{key.data()+ModelTopics::KeyPrefixOffset,key.size()-ModelTopics::KeyPrefixOffset};
    }
    return lib::string_view{};
}

}


//---------------------------------------------------------------

Result<common::pmr::set<Topic>>
    ModelTopics::modelTopics(
        const std::string &modelId,
        RocksdbHandler &handler,
        RocksdbPartition *partition,
        bool onlyDefaultPartition,
        const common::pmr::AllocatorFactory* factory
    )
{
    std::pmr::set<Topic> topics{factory->objectAllocator<Topic>()};

    // construct keys
    auto rdOpts=handler.p()->readOptions;
    KeyBuf keyFrom;
    KeyBuf keyTo;
    fillModelKeyPrefix(modelId,keyFrom);
    Slice ksFrom{keyFrom.data(),keyFrom.size()};
    fillModelKeyPrefix(modelId,keyTo,true);
    Slice ksTo{keyTo.data(),keyTo.size()};
    rdOpts.iterate_lower_bound=&ksFrom;
    rdOpts.iterate_upper_bound=&ksTo;

//! @maybe Log debug
#if 0
    std::cout << "Topics from " << logKey(ksFrom) << " to " << logKey(ksTo) << std::endl;
#endif

    auto eachPartition=[&](RocksdbPartition *partition)
    {
        std::unique_ptr<ROCKSDB_NAMESPACE::Iterator> it{
            handler.p()->db->NewIterator(
                rdOpts,
                partition->dataCf()
                )
        };
        it->SeekToFirst();
        auto hasKey=it->Valid();
        while (hasKey)
        {
            auto topic=topicFromKey(it->key());
            if (topics.find(topic)==topics.end())
            {
                Topic t;
                t.load(topic);
                topics.emplace(std::move(t));
            }

//! @maybe Log debug
#if 0
        std::cout << "Found topic \"" << topic  << "\" for key=" << logKey(it->key())
                  << " offset=" <<KeyPrefixOffset
                  << " topics.size()=" << topics.size() << std::endl;
#endif

            it->Next();
            hasKey=it->Valid();
        }
        if (!it->status().ok())
        {
            if (it->status().code()==ROCKSDB_NAMESPACE::Status::kNotFound)
            {
                return Error{OK};
            }
            return makeError(DbError::MODEL_TOPIC_RELATION_READ,it->status());
        }

        return Error{};
    };
    if (partition!=nullptr)
    {
        auto ec=eachPartition(partition);
        HATN_CHECK_EC(ec);
    }
    else
    {
        for (auto&& it: handler.partitionRanges())
        {
            if (it.isNull())
            {
                auto ec=eachPartition(handler.defaultPartition().get());
                HATN_CHECK_EC(ec);
                if (onlyDefaultPartition)
                {
                    break;
                }
            }
            else if (!onlyDefaultPartition)
            {
                auto p=handler.partition(it);
                if (p)
                {
                    auto ec=eachPartition(p.get());
                    HATN_CHECK_EC(ec);
                }
            }
        }
    }

    return topics;
}

//---------------------------------------------------------------

Error ModelTopics::deleteTopic(
        Topic topic,
        RocksdbHandler &handler,
        RocksdbPartition *partition,
        ROCKSDB_NAMESPACE::WriteBatch& batch
    )
{
    HATN_CTX_SCOPE("modeltopicsdeltopic")

    auto schema=handler.schema();
    auto dbSchema=schema->dbSchema();

    for (auto&& it: dbSchema->models())
    {
        const auto& modelId=it.second->modelIdStr();
        KeyBuf key;
        fillRelationKey(modelId,topic,key);
        Slice ks{key.data(),key.size()};

        auto status=batch.Delete(partition->dataCf(),ks);
        if (!status.ok())
        {
            HATN_CTX_SCOPE_PUSH("coll",it.second->collection())
            return makeError(DbError::MODEL_TOPIC_RELATION_DEL,status);
        }
    }

    return OK;
}

//---------------------------------------------------------------

// A topic whose count is zero is scheduled for removal by the TTL compaction filter, any other is kept.
static void settleRelationTtl(ModelTopics::Relation &rel)
{
    if (rel.count==0)
    {
        // schedule to delete using TTL back in time
        uint32_t delTp=0x1000;
        rel.ttl.fillExpireAt(delTp);
    }
    else if (!rel.ttl.isNull())
    {
        // clear TTL
        rel.ttl.clear();
    }
}

// The change of the counter that one operation makes, see ModelTopics::Delta.
static void operationToDelta(const ModelTopics::Operation& op, int64_t& change)
{
    change=0;
    if (op.op==ModelTopics::Operator::Del)
    {
        change=-1;
    }
    else if (op.op==ModelTopics::Operator::Add)
    {
        change=1;
    }
}

static void mergeRelOp(ModelTopics::Relation &rel, const ModelTopics::Operation& op)
{
    int64_t change=0;
    operationToDelta(op,change);
    if (change<0)
    {
        if (rel.count>0)
        {
            rel.count--;
        }
    }
    else if (change>0)
    {
        rel.count++;
    }

    settleRelationTtl(rel);
}

static void mergeRelDelta(ModelTopics::Relation &rel, const ModelTopics::Delta& delta)
{
    int64_t value=static_cast<int64_t>(rel.count)+delta.change;
    if (value<static_cast<int64_t>(delta.floor))
    {
        value=static_cast<int64_t>(delta.floor);
    }
    rel.count=static_cast<uint64_t>(value);

    settleRelationTtl(rel);
}

// delta1 followed by (change,floor): max(max(x+a1,b1)+a2,b2)=max(x+a1+a2,max(b1+a2,b2))
static void composeDelta(ModelTopics::Delta& delta, int64_t change, uint64_t floor)
{
    int64_t newFloor=static_cast<int64_t>(delta.floor)+change;
    if (newFloor<static_cast<int64_t>(floor))
    {
        newFloor=static_cast<int64_t>(floor);
    }
    delta.floor=static_cast<uint64_t>(newFloor);
    delta.change+=change;
}

bool MergeModelTopic::FullMergeV2(
        const MergeOperationInput &merge_in,
        MergeOperationOutput *merge_out
    ) const
{

#if 0
    std::cout << "MergeModelTopic::FullMergeV2 operand_list.size()=" << merge_in.operand_list.size()
              << " key=" << logKey(merge_in.key)
              << std::endl;
#endif

    ModelTopics::Relation rel;
    if (merge_in.existing_value!=nullptr)
    {
#if 0
        std::cout << "deserialize relation"<<std::endl;
#endif
        if (!ModelTopics::deserializeRelation(merge_in.existing_value->data(),merge_in.existing_value->size(),rel))
        {
#if 0
            std::cerr<<"failed to deserialize rel"<<std::endl;
#endif
            return false;
        }
    }

    for (auto&& it : merge_in.operand_list)
    {
        ModelTopics::Operation op;
        ModelTopics::Delta delta;
        if (ModelTopics::deserializeOperation(it.data(),it.size(),op))
        {
            mergeRelOp(rel,op);
        }
        else if (ModelTopics::deserializeDelta(it.data(),it.size(),delta))
        {
            mergeRelDelta(rel,delta);
        }
        else if (!ModelTopics::deserializeRelation(it.data(),it.size(),rel))
        {
            // A Relation as an operand was written by the versions that folded operands into an absolute count. It is
            // still taken as one (there is nothing better to do with it), no operand is ever written in that form now.
#if 0
            std::cerr<<"failed to deserialize rel-op"<<std::endl;
#endif
            return false;
        }
    }
#if 0
    std::cout << "rel.count=" << rel.count
              << " rel.ttl=" << rel.ttl.timepoint() << std::endl;
#endif

    ModelTopics::serializeRelation(rel,&merge_out->new_value);
    return true;
}

bool MergeModelTopic::PartialMergeMulti(const Slice& /*key*/,
                       const std::deque<Slice>& operand_list,
                       std::string* new_value, ROCKSDB_NAMESPACE::Logger*) const
{

#if 0
    std::cout << "MergeModelTopic::PartialMerge operand_list.size()=" << operand_list.size()
              << " key=" << logKey(key)
              << std::endl;
#endif
    // The operands are folded into one Delta, which gives the same count as applying them one by one to any value, see
    // ModelTopics::Delta. The result must never be an absolute count: the value it is applied to is not known here.
    ModelTopics::Delta folded;

    for (auto&& it : operand_list)
    {
        ModelTopics::Operation op;
        ModelTopics::Delta delta;
        if (ModelTopics::deserializeOperation(it.data(),it.size(),op))
        {
            int64_t change=0;
            operationToDelta(op,change);
            composeDelta(folded,change,0);
        }
        else if (ModelTopics::deserializeDelta(it.data(),it.size(),delta))
        {
            composeDelta(folded,delta.change,delta.floor);
        }
        else
        {
            // an operand of the old absolute form (or an unknown one) cannot be folded: leave all of them to the full merge
            return false;
        }
    }

#if 0
    std::cout << "MergeModelTopic::PartialMergeMulti change=" << folded.change
              << " floor=" << folded.floor << std::endl;
#endif
    ModelTopics::serializeDelta(folded,new_value);
    return true;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
