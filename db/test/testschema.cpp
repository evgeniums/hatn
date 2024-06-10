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

    HDB_NAME_PREPARE(m1);
    HDB_NAME_PREPARE(idx_id);
    HDB_NAME_PREPARE(idx_created_at);

} // anonymous namespace

#define HDB_MAKE_INDEX(Model,Name)


BOOST_AUTO_TEST_SUITE(TestDbSchema)

BOOST_AUTO_TEST_CASE(MakeIndex)
{
    auto idx1=makeIndex(HDB_NAME(m1),HDB_NAME(idx_id),IndexConfig<Unique>{},object::_id);
    BOOST_CHECK_EQUAL(idx1.model(),"m1");
    BOOST_CHECK_EQUAL(idx1.name(),"idx_id");
    BOOST_CHECK(idx1.unique());
    BOOST_CHECK(!idx1.date_partition());
    BOOST_CHECK(!idx1.topic());
    BOOST_CHECK_EQUAL(idx1.ttl(),0);
    BOOST_CHECK_EQUAL(hana::size(idx1.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx1.fields).name(),"_id");

    auto idx2=makeIndex(HDB_NAME(m1),HDB_NAME(idx_created_at),
                          IndexConfig<NotUnique,DatePartition,HDB_TTL(3600)>{},
                          object::created_at
                       );
    BOOST_CHECK_EQUAL(idx2.model(),"m1");
    BOOST_CHECK_EQUAL(idx2.name(),"idx_created_at");
    BOOST_CHECK(!idx2.unique());
    BOOST_CHECK(idx2.date_partition());
    BOOST_CHECK(!idx2.topic());
    BOOST_CHECK_EQUAL(idx2.ttl(),3600);
    BOOST_CHECK_EQUAL(hana::size(idx2.fields),1);
    BOOST_CHECK_EQUAL(hana::front(idx2.fields).name(),"created_at");
    BOOST_CHECK_EQUAL(idx2.date_partition_field().name(),"created_at");
}

BOOST_AUTO_TEST_SUITE_END()
