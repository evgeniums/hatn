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

#include <hatn/db/plugins/rocksdb/modeltopics.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/rocksdberror.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

//---------------------------------------------------------------

Error ModelTopics::update(
        const ModelInfo *model,
        const Topic &topic,
        RocksdbHandler &handler,
        RocksdbPartition* partition,
        bool addNremove,
        const TtlMark& ttl
    )
{
    std::array<char,MaxOperationSize> op;

    size_t size=0;
    if (addNremove)
    {
        op[0]=static_cast<char>(Operator::Add);
    }
    else
    {
        op[0]=static_cast<char>(Operator::Del);
    }
    size+=sizeof(Operator);
    size+=ttl.copy(op.data()+size,TtlMark::Size);
    ROCKSDB_NAMESPACE::Slice ops(op.data(),size);

    KeyBuf key;
    fillRelationKey(model,topic,key);
    ROCKSDB_NAMESPACE::Slice ks(key.data(),key.size());

    auto status=handler.p()->db->Merge(
            handler.p()->writeOptions,
        partition->collectionCf.get(),
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

void ModelTopics::deserializeOperation(const char *data, size_t size, Operation &op)
{
    Assert(size==(sizeof(Operation)+TtlMark::MinSize) || size==(sizeof(Operation)+TtlMark::Size),"Invalid size of ModelTopics operation");
    op.op=static_cast<Operator>(data[0]);
    op.ttl.load(data+sizeof(Operation),size-sizeof(Operation));
}

//---------------------------------------------------------------

void ModelTopics::serializeRelation(const Relation &rel, std::string *value)
{
    value->resize(sizeof(rel.version)+sizeof(rel.count)+rel.ttl.size());

    value[0]=static_cast<char>(rel.version);

    uint64_t count=boost::endian::native_to_little(rel.count);
    memcpy(value->data()+sizeof(rel.version),&count,sizeof(rel.count));
    rel.ttl.copy(value->data()+sizeof(rel.version)+sizeof(rel.count),rel.ttl.size());
}

//---------------------------------------------------------------

Error ModelTopics::deserializeRelation(const char *data, size_t size, Relation &rel)
{
    if (size!=MinRelationSize && size!=MaxRelationSize)
    {
        return dbError(DbError::MODEL_TOPIC_RELATION_DESER);
    }
    rel.version=data[0];
    uint64_t count=0;
    memcpy(&count,data+sizeof(rel.version),sizeof(rel.count));
    rel.count=boost::endian::little_to_native(count);
    rel.ttl.load(data+sizeof(rel.version)+sizeof(rel.count),size-sizeof(rel.version)-sizeof(rel.count));
    return OK;
}

//---------------------------------------------------------------

void ModelTopics::fillRelationKey(const ModelInfo *model, const Topic &topic, KeyBuf& key)
{
    key.clear();
    key.append(RelationKeyPrefix);
    key.append(SeparatorCharStr);
    key.append(model->modelIdStr());
    key.append(SeparatorCharStr);
    key.append(topic.topic());
}

//---------------------------------------------------------------

void ModelTopics::fillModelKeyPrefix(const ModelInfo *model, KeyBuf& key)
{
    key.clear();
    key.append(RelationKeyPrefix);
    key.append(SeparatorCharStr);
    key.append(model->modelIdStr());
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
