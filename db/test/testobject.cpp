/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testobject.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/test/multithreadfixture.h>

#include <hatn/common/elapsedtimer.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/db/object.h>
#include <hatn/dataunit/ipp/objectid.ipp>

#include "hatn_test_config.h"

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

HDU_UNIT_WITH(u1,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
)

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestDbObject)

BOOST_AUTO_TEST_CASE(BaseObjectInit)
{
    object::type o1;

    BOOST_TEST_MESSAGE(fmt::format("original object:\n{}",o1.toString(true)));
    initObject(o1);
    BOOST_TEST_MESSAGE(fmt::format("initialized object:\n{}",o1.toString(true)));

    WireBufSolid buf1;
    Error ec;
    io::serialize(o1,buf1,ec);
    BOOST_REQUIRE(!ec);

    object::type o2;
    io::deserialize(o2,buf1,ec);
    BOOST_REQUIRE(!ec);
    BOOST_TEST_MESSAGE(fmt::format("parsed object:\n{}",o1.toString(true)));
    BOOST_CHECK_EQUAL(o1.toString(),o2.toString());

    auto str1=o1.toString();
    object::type o3;
    auto ok=o3.loadFromJSON(str1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(o1.toString(),o3.toString());
}

BOOST_AUTO_TEST_CASE(DerivedObjectInit)
{
    u1::type o1;

    BOOST_TEST_MESSAGE(fmt::format("original object:\n{}",o1.toString(true)));
    initObject(o1);
    o1.field(u1::f1).set(12345678);
    BOOST_TEST_MESSAGE(fmt::format("initialized object:\n{}",o1.toString(true)));

    WireBufSolid buf1;
    Error ec;
    io::serialize(o1,buf1,ec);
    BOOST_REQUIRE(!ec);

    u1::type o2;
    io::deserialize(o2,buf1,ec);
    BOOST_REQUIRE(!ec);
    BOOST_TEST_MESSAGE(fmt::format("parsed object:\n{}",o1.toString(true)));
    BOOST_CHECK_EQUAL(o1.toString(),o2.toString());

    auto str1=o1.toString();
    u1::type o3;
    auto ok=o3.loadFromJSON(str1);
    BOOST_REQUIRE(ok);
    BOOST_CHECK_EQUAL(o1.toString(),o3.toString());
}

BOOST_FIXTURE_TEST_CASE(GenerateObjectId,MultiThreadFixture)
{
    ObjectId id;

    for (size_t i=0; i<320; i++)
    {
        id.generate();
        BOOST_TEST_MESSAGE(fmt::format("{}: id={} tp={:010x} seq={:06x} rand={:08x}",i,id.toString(),id.timepoint(),id.seq(),id.rand()));
    }

    BOOST_TEST_MESSAGE("Generate sequential IDs");
    size_t count=3000000;
    size_t maxSeq=0;
    for (size_t i=0; i<count; i++)
    {
        id.generate();
        if (id.seq()>maxSeq)
        {
            maxSeq=id.seq();
        }
    }
    BOOST_TEST_MESSAGE(fmt::format("Max IDs per millisecond: {}",maxSeq));
    BOOST_CHECK(maxSeq>100);

    BOOST_TEST_MESSAGE("Generate parallel IDs");
    int jobs=8;
    std::atomic<size_t> doneCount{0};
    auto handler=[&jobs,&doneCount,count,this](size_t idx)
    {
        ObjectId id;
        ObjectId maxId;
        for (size_t i=0;i<count;i++)
        {
            id.generate();
            if (id.seq()>maxId.seq())
            {
                maxId=id;
            }
        }

        HATN_TEST_MESSAGE_TS(fmt::format("Done handler for thread {}, max IDs per millisecond: {}, id with max seq: {}",idx,maxId.seq(),maxId.toString()));
        if (++doneCount==jobs)
        {
            this->quit();
        }
    };
    createThreads(jobs+1);
    thread(0)->start(false);
    for (int j=0;j<jobs;j++)
    {
        thread(j+1)->execAsync(
            [&handler,j]()
            {
                handler(j);
            }
        );
        thread(j+1)->start(false);
    }
    exec(60);
    thread(0)->stop();
    for (int j=0;j<jobs;j++)
    {
        thread(j+1)->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(InitObjectPerformance,MultiThreadFixture)
{
    size_t count=3000000;
    BOOST_TEST_MESSAGE(fmt::format("Init {} objects",count));
    size_t maxSeq=0;
    common::ElapsedTimer elapsed;
    for (size_t i=0; i<count; i++)
    {
        object::type o1;
        initObject(o1);
        auto seq=o1.field(object::_id).value().seq();
        if (seq>maxSeq)
        {
            maxSeq=seq;
        }
    }
    auto ms=elapsed.elapsedMs();
    BOOST_TEST_MESSAGE(fmt::format("Elapsed {}ms, max IDs per millisecond: {}, objects per second: {}",ms,maxSeq,(count*1000)/ms));
    BOOST_CHECK(maxSeq>100);
}

BOOST_AUTO_TEST_SUITE_END()
