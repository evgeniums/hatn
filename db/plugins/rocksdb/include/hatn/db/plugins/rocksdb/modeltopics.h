/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/modeltopics.h
  *
  *   RocksDB model-topic relations.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBMODELTOPICS_H
#define HATNROCKSDBMODELTOPICS_H

#include <hatn/dataunit/syntax.h>

#include <hatn/db/topic.h>
#include <hatn/db/model.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT ModelTopics
{
    public:

        constexpr static const uint8_t Version=1;
        constexpr static const uint8_t MaxRelationSize=sizeof(uint8_t)+sizeof(size_t)+TtlMark::Size;
        constexpr static const uint8_t MinRelationSize=sizeof(uint8_t)+sizeof(size_t)+TtlMark::MinSize;

        constexpr static const std::array<char,4> RelationKeyPrefix{InternalPrefixC,0,0,1};

        enum class Operator : uint8_t
        {
            Add,
            Del
        };

        struct Relation
        {
            uint8_t version;
            uint64_t count;
            TtlMark ttl;
        };

        struct Operation
        {
            Operator op;
            TtlMark ttl;
        };

        constexpr static const size_t MaxOperationSize=sizeof(Operator)+TtlMark::Size;

        static Error update(
            const ModelInfo* model,
            const Topic& topic,
            RocksdbHandler& handler,
            RocksdbPartition* partition,
            bool addNremove,
            const TtlMark& ttl=TtlMark{}
        );

        static Error deserializeRelation(const char* data, size_t size, Relation& rel);
        static void serializeRelation(const Relation& rel, std::string* value);

        static void deserializeOperation(const char* data, size_t size, Operation& op);

        static void fillRelationKey(
            const ModelInfo* model,
            const Topic& topic,
            KeyBuf& key
        );

        static void fillModelKeyPrefix(
            const ModelInfo* model,
            KeyBuf& key
        );
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELTOPICS_H
