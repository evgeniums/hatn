/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testschema.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/common/meta/tupletypec.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/db/object.h>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/schema.h>
#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>

#include "hatn_test_config.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

HDU_UNIT(n1,
    HDU_FIELD(f1,TYPE_DATETIME,1)
)

HDU_UNIT_WITH(nu1,(HDU_BASE(object)),
    HDU_FIELD(nf1,n1::TYPE,1)
    HDU_FIELD(f2,TYPE_UINT32,2)
)

// using Tpc=std::vector<Topic>;
using Tpc=common::FlatSet<Topic>;

class S1
{
public:

    template <typename ...Args>
    S1(Args&&... args)
        : m_topics(std::forward<Args>(args)...)
    {}

    // S1(Tpc&& topics) : m_topics(std::move(topics))
    // {}

private:

    Tpc m_topics;
};

HATN_DB_INDEX(compoundIdx,object::_id,object::created_at)

HATN_DB_INDEX(nestedIdx,nestedField(nu1::nf1,n1::f1))

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestDbSchema)

BOOST_AUTO_TEST_CASE(MakeIndexQuery)
{
    ObjectId oid1;
    oid1.generate();

    auto q0=makeQuery(oidIdx(),query::where(object::_id,query::Operator::eq,oid1));
    BOOST_CHECK_EQUAL(q0.topics().size(),0);
    BOOST_CHECK_EQUAL(q0.fields().size(),1);

    auto q1=makeQuery(oidIdx(),query::where(object::_id,query::Operator::eq,oid1),"topic1");
    BOOST_CHECK_EQUAL(q1.topics().size(),1);
    BOOST_CHECK_EQUAL(q1.fields().size(),1);

    auto q2=makeQuery(oidIdx(),query::where(object::_id,query::Operator::eq,oid1),"topic1","topic2");
    BOOST_CHECK_EQUAL(q2.topics().size(),2);
    BOOST_CHECK_EQUAL(q2.fields().size(),1);
    q2.topics().loadRange(std::vector{"topic3","topic4","topic5"});
    BOOST_REQUIRE_EQUAL(q2.topics().size(),5);
    BOOST_CHECK_EQUAL(std::string(q2.topics().at(4).topic()),std::string("topic5"));

    auto now=common::DateTime::currentUtc();
    auto q3=makeQuery(compoundIdx(),query::where(object::_id,query::Operator::eq,oid1).
                                            and_(object::created_at,query::Operator::lt,now)
                        ,"topic1");
    BOOST_CHECK_EQUAL(q3.topics().size(),1);
    BOOST_CHECK_EQUAL(q3.fields().size(),2);

    auto q4=makeQuery(nestedIdx(),query::where(nested(nu1::nf1,n1::f1),query::Operator::lt,now)
                        ,"topic1");
    BOOST_CHECK_EQUAL(q4.topics().size(),1);
    BOOST_CHECK_EQUAL(q4.fields().size(),1);
}

BOOST_AUTO_TEST_CASE(MakeIndex)
{
    ModelRegistry::free();

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    BOOST_CHECK_EQUAL(idx1.name(),"idx_id");
    BOOST_CHECK(idx1.unique());
    BOOST_CHECK(!idx1.isDatePartitioned());
    BOOST_CHECK_EQUAL(idx1.ttl(),0);
    BOOST_CHECK_EQUAL(hana::size(idx1.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx1.fields).name(),"_id");

    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    BOOST_CHECK_EQUAL(idx2.name(),"idx_created_at");
    BOOST_CHECK(!idx2.unique());
    BOOST_CHECK(idx2.isDatePartitioned());
    BOOST_CHECK_EQUAL(idx2.ttl(),3600);
    BOOST_CHECK_EQUAL(hana::size(idx2.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx2.fields).name(),"created_at");
    BOOST_CHECK_EQUAL(idx2.frontField().name(),"created_at");

    auto idx3=makeIndex(IndexConfig<NotUnique,NotDatePartition>{},object::created_at,object::updated_at);
    BOOST_CHECK_EQUAL(idx3.name(),"idx_created_at_updated_at");

    static_assert(std::is_same<decltype(object::created_at)::Type::type,common::DateTime>::value,"");

    auto idx4=makeIndex(IndexConfig<NotUnique,DatePartition>{},object::_id);
    BOOST_CHECK(!idx4.unique());
    BOOST_CHECK(idx4.isDatePartitioned());

    auto idx6=makeIndex(object::created_at,object::updated_at);
    BOOST_CHECK_EQUAL(idx6.name(),"idx_created_at_updated_at");
    BOOST_CHECK(!idx6.unique());
    BOOST_CHECK(!idx6.isDatePartitioned());
}

BOOST_AUTO_TEST_CASE(MakeModel)
{
    ModelRegistry::free();

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    auto idx3=makeIndex(IndexConfig<>{},object::updated_at);
    auto model1=unitModel<object::TYPE>(ModelConfig{"object_collection"},idx1,idx2,idx3);
    BOOST_CHECK_EQUAL(model1.collection(),"object_collection");
    BOOST_CHECK_EQUAL(model1.modelIdStr(),"222153d8");
    BOOST_CHECK_EQUAL(model1.modelId(),572609496);
    BOOST_CHECK_EQUAL(hana::at(model1.indexes,hana::size_c<2>).collection(),"object_collection");
    BOOST_CHECK_EQUAL(hana::at(model1.indexes,hana::size_c<2>).id(),"7aad584e");
    BOOST_CHECK(model1.isDatePartitioned());
    auto partitionField1=model1.datePartitionField();
    BOOST_CHECK_EQUAL(partitionField1.name(),"created_at");
    object::type o1;
    o1.field(object::created_at).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange1=datePartition(o1,model1);
    BOOST_REQUIRE(partitionRange1.isValid());
    BOOST_CHECK_EQUAL(partitionRange1.value(),32024006);

    auto model2=unitModel<object::TYPE>(idx1,idx3);
    BOOST_CHECK_EQUAL(model2.collection(),"object");
    BOOST_CHECK_EQUAL(model2.modelIdStr(),"ebf0a7b7");
    BOOST_CHECK_EQUAL(model2.modelId(),3958417335);
    BOOST_CHECK_EQUAL(hana::at(model2.indexes,hana::size_c<1>).collection(),"object");
    BOOST_CHECK_EQUAL(hana::at(model2.indexes,hana::size_c<1>).id(),"cf848495");
    BOOST_CHECK(!model2.isDatePartitioned());
    auto partitionField2=model2.datePartitionField();
    BOOST_CHECK(!partitionField2.value);
    object::type o2;
    o2.field(object::created_at).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange2=datePartition(o2,model2);
    BOOST_CHECK(!partitionRange2.isValid());

    BOOST_CHECK_EQUAL(model1.modelId(),572609496);
    BOOST_CHECK_EQUAL(model2.modelId(),3958417335);

    auto model3=unitModel<n1::TYPE>(ModelConfig{"object_collection2"});
    BOOST_CHECK_EQUAL(model3.modelId(),3528016721);

    auto idx4=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    auto model4=unitModel<object::TYPE>(ModelConfig{"object_collection3"},idx1,idx4,idx3);
    BOOST_CHECK_EQUAL(model4.modelId(),3432328208);
    BOOST_CHECK(model4.isDatePartitioned());
    auto partitionField4=model4.datePartitionField();
    BOOST_CHECK_EQUAL(partitionField4.name(),"created_at");
    object::type o4;
    o4.field(object::created_at).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange4=datePartition(o4,model4);
    BOOST_REQUIRE(partitionRange4.isValid());
    BOOST_CHECK_EQUAL(partitionRange4.value(),32024006);

    BOOST_CHECK_EQUAL(idx4.id(),std::string());
    BOOST_CHECK_EQUAL(model4.indexId(idx4),std::string("7b1ded1b"));
}

BOOST_AUTO_TEST_CASE(NestedIndexField)
{
    ModelRegistry::free();

    auto nestedField1=nestedField(nu1::nf1,n1::f1);

    constexpr auto isField=hana::or_(hana::is_a<db::FieldTag,decltype(nestedField1)>,hana::is_a<NestedFieldTag,decltype(nestedField1)>);
    static_assert(decltype(isField)::value,"");
    auto ft=hana::make_tuple(nestedField1);
    const auto& lastArg=hana::back(ft);
    constexpr auto isBackField=hana::or_(hana::is_a<db::FieldTag,decltype(lastArg)>,hana::is_a<NestedFieldTag,decltype(lastArg)>);
    static_assert(decltype(isBackField)::value,"");

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},nestedField1);
    BOOST_CHECK_EQUAL(idx2.name(),"idx_nf1__f1");
    auto idx3=makeIndex(IndexConfig<>{},object::updated_at);

    auto model1=unitModel<object::TYPE>(ModelConfig{"model1"},idx1,idx2,idx3);
    BOOST_CHECK(model1.isDatePartitioned());
    auto partitionField1=model1.datePartitionField();
    BOOST_CHECK_EQUAL(partitionField1.name(),"nf1__f1");

    nu1::type o1;
    o1.field(nu1::nf1).mutableValue()->field(n1::f1).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange1=datePartition(o1,model1);
    BOOST_REQUIRE(partitionRange1.isValid());
    BOOST_CHECK_EQUAL(partitionRange1.value(),32024006);

    std::ignore=update::request(update::field(nestedField(nu1::nf1,n1::f1),update::set,common::DateTime::currentUtc()));
}

BOOST_AUTO_TEST_CASE(DynamicCast)
{
    n1::managed sample;

    auto samplePtr=static_cast<dataunit::Unit*>(&sample);
    auto samplePtr1=common::dynamicCastWithSample(samplePtr,&sample);

    auto offset=reinterpret_cast<uintptr_t>(static_cast<dataunit::Unit*>(const_cast<n1::managed*>(&sample)))-reinterpret_cast<uintptr_t>(&sample);

    std::cerr << fmt::format("sample=0x{:x}, samplePtr=0x{:x}, offset=0x{:x}, diff=0x{:x}, samplePtr1=0x{:x}",
                             reinterpret_cast<uintptr_t>(&sample),
                             reinterpret_cast<uintptr_t>(samplePtr),
                             offset,
                             reinterpret_cast<uintptr_t>(samplePtr)-reinterpret_cast<uintptr_t>(&sample),
                             reinterpret_cast<uintptr_t>(samplePtr1)
                             ) << std::endl;

    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(&sample),reinterpret_cast<uintptr_t>(samplePtr1));

    auto objShared=common::makeShared<n1::managed>();
    BOOST_CHECK(static_cast<bool>(objShared));
    auto unitPtr=objShared.staticCast<dataunit::Unit>();
    BOOST_CHECK(static_cast<bool>(unitPtr));

    auto obj1=common::dynamicCastWithSample(unitPtr.get(),&sample);

    auto objShared1=obj1->sharedFromThis();
    BOOST_CHECK(static_cast<bool>(objShared1));

    BOOST_CHECK_EQUAL(reinterpret_cast<uintptr_t>(objShared.get()),reinterpret_cast<uintptr_t>(objShared1.get()));
}

BOOST_AUTO_TEST_SUITE_END()
