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

#include <rocksdb/merge_operator.h>

#include <hatn/common/allocatoronstack.h>
#include <hatn/common/pmr/allocatorfactory.h>

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
        constexpr static const uint8_t MaxRelationSize=sizeof(uint8_t)+sizeof(uint64_t)+TtlMark::Size;
        constexpr static const uint8_t MinRelationSize=sizeof(uint8_t)+sizeof(uint64_t)+TtlMark::MinSize;

        constexpr static const std::array<char,4> RelationKeyPrefix{InternalPrefixC,0,0,1};
        constexpr static const size_t KeyPrefixOffset=RelationKeyPrefix.size()+HATN_COMMON_NAMESPACE::Crc32HexLength+2;

        enum class Operator : uint8_t
        {
            Add,
            Del,
            Update
        };

        struct Relation
        {
            uint8_t version=Version;
            uint64_t count=0;
            TtlMark ttl;
        };

        struct Operation
        {
            Operator op=Operator::Add;
        };

        constexpr static const size_t MaxOperationSize=sizeof(Operator)+TtlMark::Size;

        /**
         * @brief Several operations folded into one operand by PartialMergeMulti().
         *
         * The counter never goes below zero, so a run of operations acts on the counter x as x -> max(x+change, floor).
         * Such functions compose (max(max(x+a1,b1)+a2,b2)=max(x+a1+a2,max(b1+a2,b2))), and that is what lets RocksDB fold
         * the operands at a flush or a compaction, long before the value they apply to is at hand, and still get the
         * same count as applying them one by one. The size differs from both an Operation (1 byte) and a Relation
         * (MinRelationSize..MaxRelationSize), so the three kinds of operands tell themselves apart by size.
         */
        struct Delta
        {
            int64_t change=0;
            uint64_t floor=0;
        };
        constexpr static const uint8_t DeltaMarker=0xD2;
        constexpr static const size_t DeltaSize=sizeof(uint8_t)+sizeof(int64_t)+sizeof(uint64_t);

        static Error update(
            const std::string& modelId,
            Topic topic,
            RocksdbHandler& handler,
            RocksdbPartition* partition,
            Operator action
        );

        static Result<common::pmr::set<Topic>> modelTopics(
            const std::string& modelId,
            RocksdbHandler& handler,
            RocksdbPartition* partition=nullptr,
            bool onlyDefaultPartition=false,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        /**
         * @brief Put back the topics of a model that have objects but are missing from the registry or are zero in it.
         *
         * A query without a topic looks in the topics of the registry only, so an object whose topic is not in it is not
         * found by such a query although the object and its index entries are intact. The registry is derived data: this
         * rebuilds what is missing from the objects. Only the topics that the registry does not list as non-empty are
         * counted (one point lookup per topic of the database and one seek, objects of the other topics are not touched),
         * and the count is merged as a Delta, so an Add or a Del that is made at the same time is not lost.
         *
         * @param model Model to reconcile.
         * @param handler Rocksdb handler.
         * @return Number of topics that were put back.
         */
        static Result<size_t> reconcile(
            const ModelInfo& model,
            RocksdbHandler& handler
        );

        static Error deleteTopic(
            Topic topic,
            RocksdbHandler& handler,
            RocksdbPartition* partition,
            ROCKSDB_NAMESPACE::WriteBatch& batch
        );

        /**
         * @brief count
         * @param model
         * @param topic
         * @param date
         * @param handler
         * @return
         *
         * @note Can be very slow for topics with large rate of objects inserts/deletes because Merge
         * reconstructs a value from the queue of updates on every Get until compaction.
         * Compaction also can be slow.
         *
         * @todo optimization: Maybe it would be better to reimplement it using transactions
         * instead of Merge.
         */
        static Result<size_t> count(
            const ModelInfo& model,
            Topic topic,
            const HATN_COMMON_NAMESPACE::Date& date,
            RocksdbHandler& handler
        );

        static bool deserializeRelation(const char* data, size_t size, Relation& rel);
        static void serializeRelation(const Relation& rel, std::string* value);

        static bool deserializeOperation(const char* data, size_t size, Operation& op);

        static void serializeDelta(const Delta& delta, std::string* value);
        static bool deserializeDelta(const char* data, size_t size, Delta& delta);

        static void fillRelationKey(
            const std::string& modelId,
            Topic topic,
            KeyBuf& key
        );

        static void fillModelKeyPrefix(
            const std::string& modelId,
            KeyBuf& key,
            bool to=false
        );
};

class HATN_ROCKSDB_SCHEMA_EXPORT MergeModelTopic : public ROCKSDB_NAMESPACE::MergeOperator
{
    public:

        virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                                 MergeOperationOutput* merge_out) const override;

        virtual bool PartialMergeMulti(const ROCKSDB_NAMESPACE::Slice& key,
                                       const std::deque<ROCKSDB_NAMESPACE::Slice>& operand_list,
                                       std::string* new_value, ROCKSDB_NAMESPACE::Logger*) const override;


        virtual bool AllowSingleOperand() const override { return true; }

        virtual const char* Name() const override
        {
            return "MergeModelTopic";
        }
};


HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELTOPICS_H
