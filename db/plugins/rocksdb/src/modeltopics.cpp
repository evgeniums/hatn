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

    auto eachPartition=[multipleTopics,&eachTopic,&handler,&ks,&ksTo,&rdOpts,&model](std::shared_ptr<RocksdbPartition>& partition)
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

static lib::string_view topicFromKey(const Slice& key)
{
    if (key.size()>ModelTopics::KeyPrefixOffset)
    {
        return lib::string_view{key.data()+ModelTopics::KeyPrefixOffset,key.size()-ModelTopics::KeyPrefixOffset};
    }
    return lib::string_view{};
}


//---------------------------------------------------------------

Result<common::pmr::set<TopicHolder>>
    ModelTopics::modelTopics(
        const std::string &modelId,
        RocksdbHandler &handler,
        RocksdbPartition *partition,
        bool onlyDefaultPartition,
        const common::pmr::AllocatorFactory* factory
    )
{
    std::pmr::set<TopicHolder> topics{factory->objectAllocator<TopicHolder>()};

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
            topics.insert(TopicHolder{topic});

//! @maybe Log debug
#if 0
        std::cout << "Found topic\"" << topic  << "\" for key=" << logKey(it->key())
                  << " offset=" <<KeyPrefixOffset << std::endl;
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

static void mergeRelOp(ModelTopics::Relation &rel, const ModelTopics::Operation& op)
{
    if (op.op==ModelTopics::Operator::Del)
    {
        if (rel.count>0)
        {
            rel.count--;
        }
    }
    else if (op.op==ModelTopics::Operator::Add)
    {
        rel.count++;
    }

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
        if (!ModelTopics::deserializeOperation(it.data(),it.size(),op))
        {
#if 0
            std::cout<<"try to deserialize rel size=" << it.size() <<std::endl;
#endif
            if (!ModelTopics::deserializeRelation(it.data(),it.size(),rel))
            {
#if 0
                std::cerr<<"failed to deserialize rel-op"<<std::endl;
#endif
                return false;
            }
        }
        else
        {
            mergeRelOp(rel,op);
        }
    }
#if 0
    std::cout << "rel.count=" << rel.count
              << " rel.ttl=" << rel.ttl.timepoint() << std::endl;
#endif

    ModelTopics::serializeRelation(rel,&merge_out->new_value);
    return true;
}

bool MergeModelTopic::PartialMergeMulti(const Slice& key,
                       const std::deque<Slice>& operand_list,
                       std::string* new_value, ROCKSDB_NAMESPACE::Logger*) const
{

#if 0
    std::cout << "MergeModelTopic::PartialMerge operand_list.size()=" << operand_list.size()
              << " key=" << logKey(key)
              << std::endl;
#endif
    ModelTopics::Relation rel;

    for (auto&& it : operand_list)
    {
        ModelTopics::Operation op;
        if (!ModelTopics::deserializeOperation(it.data(),it.size(),op))
        {
            if (!ModelTopics::deserializeRelation(it.data(),it.size(),rel))
            {
                HATN_CTX_ERROR(dbError(DbError::MODEL_TOPIC_RELATION_DESER),"failed to deserialize rel-op")
                return false;
            }
        }
        else
        {
            mergeRelOp(rel,op);
        }
    }

#if 0
    std::cout << "MergeModelTopic::PartialMergeMulti rel.count=" << rel.count
              << " rel.ttl=" << rel.ttl.timepoint() << std::endl;
#endif
    ModelTopics::serializeRelation(rel,new_value);
    return true;
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
