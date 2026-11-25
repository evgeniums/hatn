/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testmideltopics.cpp
*/

/****************************************************************************/

#include <cmath>
#include <cstdlib>
#include <deque>
#include <tuple>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>
#include <hatn/common/format.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/base/configtreeloader.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/modeltopics.h>
#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

HDU_UNIT(base_fields,
         HDU_FIELD(df2,TYPE_UINT32,2)
         HDU_FIELD(df3,TYPE_STRING,3)
         )

HDU_UNIT_WITH(no_p,(HDU_BASE(object),HDU_BASE(base_fields))
              )

HDU_UNIT_WITH(implicit_p,(HDU_BASE(object),HDU_BASE(base_fields))
              )

HATN_DB_INDEX(df2Idx,base_fields::df2)
HATN_DB_INDEX(df3Idx,base_fields::df3)

HATN_DB_MODEL_WITH_CFG(modelNoP1,no_p,ModelConfig("no_p1"),df2Idx(),df3Idx())
HATN_DB_MODEL_WITH_CFG(modelNoP2,no_p,ModelConfig("no_p2"),df2Idx(),df3Idx())
HATN_DB_MODEL_WITH_CFG(modelNoP3,no_p,ModelConfig("no_p3"),df2Idx(),df3Idx())
HATN_DB_MODEL_WITH_CFG(modelNoP4,no_p,ModelConfig("no_p4"),df2Idx(),df3Idx())

HATN_DB_OID_PARTITION_MODEL_WITH_CFG(modelImplicit1,implicit_p,ModelConfig("implicit_p1"),df2Idx(),df3Idx())
HATN_DB_OID_PARTITION_MODEL_WITH_CFG(modelImplicit2,implicit_p,ModelConfig("implicit_p2"),df2Idx(),df3Idx())

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(modelNoP1());
    rdb::RocksdbModels::instance().registerModel(modelNoP2());
    rdb::RocksdbModels::instance().registerModel(modelNoP3());
    rdb::RocksdbModels::instance().registerModel(modelNoP4());
    rdb::RocksdbModels::instance().registerModel(modelImplicit1());
    rdb::RocksdbModels::instance().registerModel(modelImplicit2());
#endif
}

void init()
{
    ModelRegistry::free();
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::free();
    rdb::RocksdbModels::free();
#endif

    registerModels();
}

template <typename ...Models>
auto initSchema(Models&& ...models)
{
    auto schema1=makeSchema("schema1",std::forward<Models>(models)...);

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbSchemas::instance().registerSchema(schema1);
#endif

    return schema1;
}

template <typename T>
void setSchemaToClient(std::shared_ptr<Client> client, const T& schema)
{
    auto ec=client->setSchema(schema);
    BOOST_REQUIRE(!ec);
    auto s=client->schema();
    BOOST_REQUIRE(!s);
    BOOST_CHECK_EQUAL(s->get()->name(),schema->name());
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestModelTopics, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(NotPartitioned)
{
    init();

    auto s1=initSchema(modelNoP1(),modelNoP2(),modelNoP3(),modelNoP4());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        constexpr const size_t count=20;
        std::vector<Topic> topics{
            "topic0","topic1", "topic2","topic3"
        };
        bool ok=true;
        std::vector<ObjectId> oids;
        size_t handlerCount=0;

        // create objects
        auto createObjects=[&handlerCount,&oids,&client,&topics,&ok](const auto& m, size_t count, size_t topicsCount)
        {
            for (size_t i=0;i<count;i++)
            {
                auto o=makeInitObject<no_p::type>();
                o.setFieldValue(base_fields::df2,i);
                auto ec=client->create(topics[i%topicsCount],m,&o);
                if (ec)
                {
                    HATN_CTX_ERROR(ec,"failed to create object")
                    ok=false;
                    break;
                }
                if (handlerCount==0 && i%topicsCount==0)
                {
                    oids.push_back(o.fieldValue(Oid));
                }
            }
            BOOST_REQUIRE(ok);
            handlerCount++;
        };
        createObjects(modelNoP1(),count,topics.size());
        createObjects(modelNoP2(),count+20,topics.size()/2);
        createObjects(modelNoP3(),count+30,1);

        // count models
        auto r1=client->count(modelNoP1());
        if (r1)
        {
            HATN_CTX_ERROR(r1.error(),"failed to count modelNoP1")
        }
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count);
        auto r2=client->count(modelNoP2());
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value(),count+20);
        auto r3=client->count(modelNoP3());
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(r3.value(),count+30);
        auto r4=client->count(modelNoP4());
        BOOST_REQUIRE(!r4);
        BOOST_CHECK_EQUAL(r4.value(),0);

        // count models per topic
        r1=client->count(modelNoP1(),topics[0]);
        if (r1)
        {
            HATN_CTX_ERROR(r1.error(),"failed to count modelNoP1 topics[0]")
        }
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count/topics.size());
        r1=client->count(modelNoP1(),topics[1]);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count/topics.size());
        r1=client->count(modelNoP1(),topics[2]);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count/topics.size());
        r1=client->count(modelNoP1(),topics[3]);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count/topics.size());

        r2=client->count(modelNoP2(),topics[0]);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value(),(count+20)/(topics.size()/2));
        r2=client->count(modelNoP2(),topics[1]);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value(),(count+20)/(topics.size()/2));
        r2=client->count(modelNoP2(),topics[2]);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value(),0);
        r2=client->count(modelNoP2(),topics[3]);
        BOOST_REQUIRE(!r2);
        BOOST_CHECK_EQUAL(r2.value(),0);

        r3=client->count(modelNoP3(),topics[0]);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(r3.value(),(count+30));
        r3=client->count(modelNoP2(),topics[1]);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(r2.value(),0);
        r3=client->count(modelNoP3(),topics[2]);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(r2.value(),0);
        r3=client->count(modelNoP3(),topics[3]);
        BOOST_REQUIRE(!r3);
        BOOST_CHECK_EQUAL(r3.value(),0);

        // update some objects
        Error ec;
        size_t updateCount=5;
        auto updateReq=update::request(update::field(base_fields::df3,update::set,"Hi!"));
        for (size_t i=0;i<updateCount;i++)
        {
            ec=client->update(topics[0],modelNoP1(),oids[i],updateReq);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to update object")
                break;
            }
        }
        r1=client->count(modelNoP1());
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count);
        r1=client->count(modelNoP1(),topics[0]);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count/topics.size());

        // delete some objects
        size_t delCount=4;
        for (size_t i=0;i<delCount;i++)
        {
            ec=client->deleteObject(topics[0],modelNoP1(),oids[i]);
            if (ec)
            {
                HATN_CTX_ERROR(ec,"failed to delete object")
                break;
            }
        }
        BOOST_REQUIRE(!ec);
        r1=client->count(modelNoP1(),topics[0]);
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),(count/topics.size())-delCount);
        r1=client->count(modelNoP1());
        BOOST_REQUIRE(!r1);
        BOOST_CHECK_EQUAL(r1.value(),count - delCount);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(Partitions)
{
    init();

    auto s1=initSchema(modelImplicit1(),modelImplicit2());

    std::vector<ModelInfo> modelInfos{*(modelImplicit1()->info),*(modelImplicit2()->info)};
    std::vector<common::Date> dates{
        {2024,1,15},{2024,2,15},{2024,3,15},
        {2024,4,15},{2024,5,15},{2024,6,15},
        {2024,7,15},{2024,8,15},{2024,9,15},
        {2024,10,15},{2024,11,15},{2024,12,15}
    };

    auto handler=[&](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        std::vector<Topic> topics{"topic1","topic2","topic3","topic4"};

        setSchemaToClient(client,s1);

        auto ec=client->addDatePartitions(modelInfos,dates.back(),dates.front());
        BOOST_REQUIRE(!ec);

        // fill db with objects
        auto fill=[&](const auto& model, size_t objectsPerPartition)
        {
            size_t count=dates.size()*objectsPerPartition;

            for (size_t j=0;j<topics.size();j++)
            {
                std::map<size_t,size_t> partitionCounts;
                size_t maxPartitionCount=objectsPerPartition-j*2;
                for (size_t i=0;i<count;i++)
                {
                    auto partition=i%dates.size();
                    auto it=partitionCounts.find(partition);
                    if (it!=partitionCounts.end())
                    {
                        auto partitionCount=it->second;
                        if (partition==2 && partitionCount==20)
                        {
                            continue;
                        }
                        if (partitionCount==maxPartitionCount)
                        {
                            continue;
                        }
                        partitionCount++;
                        partitionCounts[partition]=partitionCount;
                    }
                    else
                    {
                        partitionCounts[partition]=1;
                    }

                    auto o=makeInitObject<implicit_p::type>();
                    o.setFieldValue(base_fields::df2,static_cast<uint32_t>(i));
                    o.setFieldValue(base_fields::df3,fmt::format("value_{:04d}",i));
                    common::DateTime dt{dates[partition],common::Time{1,1,1}};
                    auto oid=o.fieldValue(Oid);
                    oid.setDateTime(dt);
                    o.setFieldValue(Oid,oid);
                    o.setFieldValue(object::created_at,dt);
                    o.setFieldValue(object::updated_at,dt);
                    ec=client->create(topics[j],model,&o);
                    if (ec)
                    {
                        HATN_CTX_ERROR_RECORDS(ec,"failed to create object",{"i",i})
                        break;
                    }
                }
                if (ec)
                {
                    break;
                }
            }
            BOOST_REQUIRE(!ec);
        };
        size_t objectsPerPartition1=80;
        fill(modelImplicit1(),objectsPerPartition1);
        size_t objectsPerPartition2=60;
        fill(modelImplicit2(),objectsPerPartition2);

        // check counts for modelImplicit1()
        auto check=[&](const auto& model, size_t objectsPerPartition)
        {
            // first partition, all topics
            auto r1=client->count(model,dates[0]);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1.value(),4*objectsPerPartition-12);
            // first partition, all topic 0
            auto r2=client->count(model,dates[0],topics[0]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),objectsPerPartition);
            // first partition, topic 1
            r2=client->count(model,dates[0],topics[1]);
            BOOST_REQUIRE(!r2);
            // first partition, topic 2
            BOOST_CHECK_EQUAL(r2.value(),objectsPerPartition-2);
            r2=client->count(model,dates[0],topics[2]);
            BOOST_REQUIRE(!r2);
            // first partition, topic 3
            BOOST_CHECK_EQUAL(r2.value(),objectsPerPartition-4);
            r2=client->count(model,dates[0],topics[3]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),objectsPerPartition-6);

            // partition 2, all topics
            r1=client->count(model,dates[2]);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1.value(),20*topics.size());
            // partition 2, topic 0
            r2=client->count(model,dates[2],topics[0]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),20);
            // partition 2, topic 1
            r2=client->count(model,dates[2],topics[1]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),20);
            // partition 2, topic 2
            r2=client->count(model,dates[2],topics[2]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),20);
            // partition 2, topic 3
            r2=client->count(model,dates[2],topics[3]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),20);

            // all partitions, all topics
            r1=client->count(model);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1.value(),(4*objectsPerPartition-12)*(dates.size()-1)+20*topics.size());
            // all partitions, topic 0
            r2=client->count(model,topics[0]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),objectsPerPartition *(dates.size()-1) + 20);
            // all partitions, topic 1
            r2=client->count(model,topics[1]);
            BOOST_REQUIRE(!r2);
            // all partitions, topic 2
            BOOST_CHECK_EQUAL(r2.value(),(objectsPerPartition-2) * (dates.size()-1) + 20);
            r2=client->count(model,topics[2]);
            BOOST_REQUIRE(!r2);
            // all partitions, topic 3
            BOOST_CHECK_EQUAL(r2.value(),(objectsPerPartition-4) * (dates.size()-1) + 20);
            r2=client->count(model,topics[3]);
            BOOST_REQUIRE(!r2);
            BOOST_CHECK_EQUAL(r2.value(),(objectsPerPartition-6) * (dates.size()-1) + 20);
        };

        BOOST_TEST_CONTEXT("check modelImplicit1"){check(modelImplicit1(),objectsPerPartition1);}
        BOOST_TEST_CONTEXT("check modelImplicit2"){check(modelImplicit2(),objectsPerPartition2);}
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

namespace {

using MergeOp=ROCKSDB_NAMESPACE::MergeOperator;

std::string topicOperation(rdb::ModelTopics::Operator op)
{
    return std::string(1,static_cast<char>(op));
}

std::string topicRelation(uint64_t count)
{
    rdb::ModelTopics::Relation rel;
    rel.count=count;
    if (count==0)
    {
        // the same marker mergeRelOp() puts on a relation that dropped to zero
        rel.ttl.fillExpireAt(0x1000);
    }
    std::string value;
    rdb::ModelTopics::serializeRelation(rel,&value);
    return value;
}

// What RocksDB does when it reads the key: the base value (if any) and the pending operands are folded by FullMergeV2.
bool fullMergeTopic(const std::string* base, const std::vector<std::string>& operands, rdb::ModelTopics::Relation& result)
{
    rdb::MergeModelTopic merge;

    std::vector<ROCKSDB_NAMESPACE::Slice> slices;
    for (const auto& op : operands)
    {
        slices.emplace_back(op);
    }
    ROCKSDB_NAMESPACE::Slice key{"topic-relation-key"};
    ROCKSDB_NAMESPACE::Slice baseSlice;
    if (base!=nullptr)
    {
        baseSlice=ROCKSDB_NAMESPACE::Slice{*base};
    }
    MergeOp::MergeOperationInput in{key,base!=nullptr ? &baseSlice : nullptr,slices,nullptr};
    std::string out;
    ROCKSDB_NAMESPACE::Slice existing;
    MergeOp::MergeOperationOutput outp{out,existing};
    if (!merge.FullMergeV2(in,&outp))
    {
        return false;
    }
    if (!outp.existing_operand.empty())
    {
        out=outp.existing_operand.ToString();
    }
    return rdb::ModelTopics::deserializeRelation(out.data(),out.size(),result);
}

// What RocksDB does at a flush or compaction that has no base value at hand: the operands are folded into one operand.
bool partialMergeTopic(const std::vector<std::string>& operands, std::string& folded)
{
    rdb::MergeModelTopic merge;
    std::deque<ROCKSDB_NAMESPACE::Slice> slices;
    for (const auto& op : operands)
    {
        slices.emplace_back(op);
    }
    ROCKSDB_NAMESPACE::Slice key{"topic-relation-key"};
    return merge.PartialMergeMulti(key,slices,&folded,nullptr);
}

}

/**
 * The registry of topics of a model counts its objects per topic with RocksDB Merge operands, and a topic whose count reaches
 * zero is dropped (an expired relation is removed by the TTL compaction filter): a query without a topic then no longer
 * finds the objects of that topic, although they are all in the database. A merge operator must give the same result when
 * RocksDB folds some of the operands first (PartialMergeMulti) and applies the folded one to the base later (FullMergeV2):
 * that happens at every flush and compaction.
 */
BOOST_AUTO_TEST_CASE(MergeOperatorFoldsLikeSingleOperands)
{
    using Op=rdb::ModelTopics::Operator;
    const std::vector<Op> alphabet{Op::Add,Op::Del};
    const std::vector<int64_t> bases{-1,0,1,3};

    std::vector<std::string> failures;
    size_t checked=0;

    std::vector<std::vector<Op>> sequences;
    sequences.push_back({});
    for (size_t len=1;len<=5;len++)
    {
        const auto prev=sequences;
        for (const auto& s : prev)
        {
            if (s.size()+1==len)
            {
                for (auto op : alphabet)
                {
                    auto next=s;
                    next.push_back(op);
                    sequences.push_back(std::move(next));
                }
            }
        }
    }

    for (const auto& sequence : sequences)
    {
        if (sequence.size()<2)
        {
            continue;
        }
        std::vector<std::string> operands;
        std::string text;
        for (auto op : sequence)
        {
            operands.push_back(topicOperation(op));
            text+=(op==Op::Add?"A":"D");
        }

        for (auto base : bases)
        {
            std::string baseValue;
            if (base>=0)
            {
                baseValue=topicRelation(static_cast<uint64_t>(base));
            }
            const std::string* basePtr=base>=0 ? &baseValue : nullptr;

            rdb::ModelTopics::Relation direct;
            BOOST_REQUIRE(fullMergeTopic(basePtr,operands,direct));

            // fold the first k operands, apply the folded one and the rest to the base
            for (size_t k=2;k<=operands.size();k++)
            {
                std::vector<std::string> head(operands.begin(),operands.begin()+k);
                std::string folded;
                BOOST_REQUIRE(partialMergeTopic(head,folded));

                std::vector<std::string> mixed;
                mixed.push_back(folded);
                mixed.insert(mixed.end(),operands.begin()+k,operands.end());

                rdb::ModelTopics::Relation viaPartial;
                BOOST_REQUIRE(fullMergeTopic(basePtr,mixed,viaPartial));

                checked++;
                if (direct.count!=viaPartial.count || direct.ttl.isNull()!=viaPartial.ttl.isNull())
                {
                    failures.push_back(fmt::format("base={} ops={} folded_first={}: one by one count={} expired={}, folded count={} expired={}",
                                                   base,text,k,direct.count,!direct.ttl.isNull(),viaPartial.count,!viaPartial.ttl.isNull()));
                }
            }
        }
    }

    std::string report;
    for (size_t i=0;i<failures.size() && i<10;i++)
    {
        report+="\n  "+failures[i];
    }
    BOOST_CHECK_MESSAGE(failures.empty(),
                        fmt::format("{} of {} merge folds changed the count of the topic relation, first ones:{}",failures.size(),checked,report));
}

/**
 * Del is merged into the registry at once, inside the transaction, so a transaction that is rolled back after a delete leaves
 * the registry one object short. A topic whose objects are all deleted in rolled-back transactions would be dropped.
 */
BOOST_AUTO_TEST_CASE(RolledBackDeleteKeepsTopicCount)
{
    init();

    auto s1=initSchema(modelNoP1());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        std::ignore=plugin;
        setSchemaToClient(client,s1);

        Topic topic{"topic0"};
        std::vector<ObjectId> oids;
        for (size_t i=0;i<2;i++)
        {
            auto o=makeInitObject<no_p::type>();
            o.setFieldValue(base_fields::df2,i);
            auto ec=client->create(topic,modelNoP1(),&o);
            BOOST_REQUIRE(!ec);
            oids.push_back(o.fieldValue(Oid));
        }
        auto r=client->count(modelNoP1(),topic);
        BOOST_REQUIRE(!r);
        BOOST_CHECK_EQUAL(r.value(),2);

        // delete in a transaction that fails afterwards
        auto failing=[&](Transaction* tx)->Error
        {
            auto ec=client->deleteObject(topic,modelNoP1(),oids[0],tx);
            if (ec)
            {
                return ec;
            }
            return commonError(CommonError::GENERIC_ERROR);
        };
        auto ec=client->transaction(failing);
        BOOST_CHECK(ec);

        // the object is still there ...
        auto rRead=client->read(topic,modelNoP1(),oids[0]);
        BOOST_CHECK(!rRead);

        // ... and so is the topic registration of it
        r=client->count(modelNoP1(),topic);
        BOOST_REQUIRE(!r);
        BOOST_CHECK_EQUAL(r.value(),2);
        r=client->count(modelNoP1());
        BOOST_REQUIRE(!r);
        BOOST_CHECK_EQUAL(r.value(),2);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

/**
 * A topic that is missing from the registry hides its objects from a query without a topic. The registry is rebuilt from the objects:
 * by reconcileModelTopics(), and by a lookup without a topic that finds nothing.
 */
BOOST_AUTO_TEST_CASE(ReconcileRestoresDroppedTopic)
{
    init();

    auto s1=initSchema(modelNoP1());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        std::ignore=plugin;
        setSchemaToClient(client,s1);

        const Topic topic0{"topic0"};
        const Topic topic1{"topic1"};
        auto createObjects=[&client](const Topic& topic, uint32_t first, size_t count)
        {
            for (size_t i=0;i<count;i++)
            {
                auto o=makeInitObject<no_p::type>();
                o.setFieldValue(base_fields::df2,first+static_cast<uint32_t>(i));
                auto ec=client->create(topic,modelNoP1(),&o);
                BOOST_REQUIRE(!ec);
            }
        };
        createObjects(topic0,100,5);
        createObjects(topic1,200,3);

        auto listedTopics=[&client]()
        {
            auto r=client->listModelTopics(modelNoP1());
            BOOST_REQUIRE(!r);
            return r.value().size();
        };
        auto countAll=[&client]()
        {
            auto r=client->count(modelNoP1());
            BOOST_REQUIRE(!r);
            return r.value();
        };

        BOOST_CHECK_EQUAL(listedTopics(),2);
        BOOST_CHECK_EQUAL(countAll(),8);

        // nothing is put back while the registry is right
        auto none=client->reconcileModelTopics(modelNoP1());
        BOOST_REQUIRE(!none);
        BOOST_CHECK_EQUAL(none.value(),0);

        // the registry entry of topic0 is lost, its objects stay
        auto ec=client->deleteModelTopicRegistration(modelNoP1(),topic0);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(listedTopics(),1);
        BOOST_CHECK_EQUAL(countAll(),3);

        // put back from the objects, with the right count
        auto fixed=client->reconcileModelTopics(modelNoP1());
        BOOST_REQUIRE(!fixed);
        BOOST_CHECK_EQUAL(fixed.value(),1);
        BOOST_CHECK_EQUAL(listedTopics(),2);
        BOOST_CHECK_EQUAL(countAll(),8);
        auto c0=client->count(modelNoP1(),topic0);
        BOOST_REQUIRE(!c0);
        BOOST_CHECK_EQUAL(c0.value(),5);
        auto c1=client->count(modelNoP1(),topic1);
        BOOST_REQUIRE(!c1);
        BOOST_CHECK_EQUAL(c1.value(),3);

        // and once more it changes nothing
        auto again=client->reconcileModelTopics(modelNoP1());
        BOOST_REQUIRE(!again);
        BOOST_CHECK_EQUAL(again.value(),0);
        BOOST_CHECK_EQUAL(countAll(),8);

        // a lookup without a topic that finds nothing repairs the registry by itself
        ec=client->deleteModelTopicRegistration(modelNoP1(),topic0);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(listedTopics(),1);
        auto q=makeQuery(df2Idx(),query::where(base_fields::df2,query::eq,100u));
        auto found=client->findOne(modelNoP1(),q);
        if (found)
        {
            BOOST_TEST_MESSAGE(found.error().message());
        }
        BOOST_REQUIRE(!found);
        BOOST_REQUIRE(!found.value().isNull());
        BOOST_CHECK_EQUAL(found.value()->fieldValue(base_fields::df2),100);
        BOOST_CHECK_EQUAL(listedTopics(),2);
        BOOST_CHECK_EQUAL(countAll(),8);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

#endif

BOOST_AUTO_TEST_SUITE_END()
