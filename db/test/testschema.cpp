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
#include <hatn/db/ipp/objectid.ipp>

#include <hatn/db/schema.h>

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

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestDbSchema)

BOOST_AUTO_TEST_CASE(MakeIndex)
{
    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    BOOST_CHECK_EQUAL(idx1.name(),"idx_id");
    BOOST_CHECK(idx1.unique());
    BOOST_CHECK(!idx1.isDatePartitioned());
    BOOST_CHECK(!idx1.topic());
    BOOST_CHECK_EQUAL(idx1.ttl(),0);
    BOOST_CHECK_EQUAL(hana::size(idx1.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx1.fields).name(),"_id");

    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    BOOST_CHECK_EQUAL(idx2.name(),"idx_created_at");
    BOOST_CHECK(!idx2.unique());
    BOOST_CHECK(idx2.isDatePartitioned());
    BOOST_CHECK(!idx2.topic());
    BOOST_CHECK_EQUAL(idx2.ttl(),3600);
    BOOST_CHECK_EQUAL(hana::size(idx2.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx2.fields).name(),"created_at");
    BOOST_CHECK_EQUAL(idx2.datePartitionField().name(),"created_at");

    auto idx3=makeIndex(IndexConfig<NotUnique,NotDatePartition>{},object::created_at,object::updated_at);
    BOOST_CHECK_EQUAL(idx3.name(),"idx_created_at_updated_at");

    static_assert(std::is_same<decltype(object::created_at)::Type::type,common::DateTime>::value,"");

    auto idx4=makeIndex(IndexConfig<NotUnique,DatePartition>{},object::_id);
    BOOST_CHECK(!idx4.unique());
    BOOST_CHECK(idx4.isDatePartitioned());
}

BOOST_AUTO_TEST_CASE(MakeModel)
{
    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},object::created_at);
    auto idx3=makeIndex(IndexConfig<>{},object::updated_at);
    auto model1=makeModel<object::TYPE>(ModelConfig<>{},idx1,idx2,idx3);
    BOOST_CHECK(model1.isDatePartitioned());
    auto partitionField1=model1.datePartitionField();
    BOOST_CHECK_EQUAL(partitionField1.name(),"created_at");
    object::type o1;
    o1.field(object::created_at).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange1=datePartition(o1,model1);
    BOOST_REQUIRE(partitionRange1.isValid());
    BOOST_CHECK_EQUAL(partitionRange1.value(),32024006);

    auto model2=makeModel<object::TYPE>(ModelConfig<>{},idx1,idx3);
    BOOST_CHECK(!model2.isDatePartitioned());
    auto partitionField2=model2.datePartitionField();
    BOOST_CHECK(!partitionField2.value);
    object::type o2;
    o2.field(object::created_at).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange2=datePartition(o2,model2);
    BOOST_CHECK(!partitionRange2.isValid());

    BOOST_CHECK_EQUAL(model1.modelId(),1);
    BOOST_CHECK_EQUAL(model2.modelId(),1);

    auto model3=makeModel<n1::TYPE>(ModelConfig<>{});
    BOOST_CHECK_EQUAL(model3.modelId(),2);
}

BOOST_AUTO_TEST_CASE(NestedIndexField)
{
    auto nestedField1=nestedIndexField(nu1::nf1,n1::f1);

    constexpr auto isField=hana::or_(hana::is_a<FieldTag,decltype(nestedField1)>,hana::is_a<NestedFieldTag,decltype(nestedField1)>);
    static_assert(decltype(isField)::value,"");
    auto ft=hana::make_tuple(nestedField1);
    const auto& lastArg=hana::back(ft);
    constexpr auto isBackField=hana::or_(hana::is_a<FieldTag,decltype(lastArg)>,hana::is_a<NestedFieldTag,decltype(lastArg)>);
    static_assert(decltype(isBackField)::value,"");

    auto idx1=makeIndex(IndexConfig<Unique>{},object::_id,"idx_id");
    auto idx2=makeIndex(IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},nestedField1);
    BOOST_CHECK_EQUAL(idx2.name(),"idx_nf1__f1");
    auto idx3=makeIndex(IndexConfig<>{},object::updated_at);

    auto model1=makeModel<object::TYPE>(ModelConfig<>{},idx1,idx2,idx3);
    BOOST_CHECK(model1.isDatePartitioned());
    auto partitionField1=model1.datePartitionField();
    BOOST_CHECK_EQUAL(partitionField1.name(),"nf1__f1");

    nu1::type o1;
    o1.field(nu1::nf1).mutableValue()->field(n1::f1).set(common::DateTime{common::Date{2024,6,27},common::Time{10,1,1}});
    auto partitionRange1=datePartition(o1,model1);
    BOOST_REQUIRE(partitionRange1.isValid());
    BOOST_CHECK_EQUAL(partitionRange1.value(),32024006);
}

BOOST_AUTO_TEST_SUITE_END()
