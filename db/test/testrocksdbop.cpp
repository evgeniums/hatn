/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testrocksdbop.cpp
*/

/****************************************************************************/

#include <cmath>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <hatn/common/format.h>

#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

#include <rocksdb/db.h>

#include <hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp>

HATN_COMMON_USING
HATN_ROCKSDB_USING

#endif

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

namespace {

rocksdb::DB* openDatabase(const std::string dbPath)
{
    rocksdb::Options options;

    // destroy database if existed
    auto status = rocksdb::DestroyDB(dbPath,options);
    if (!status.ok())
    {
        BOOST_TEST_MESSAGE(fmt::format("Failed to destroy database: {}",status.ToString()));
    }

    // open/create database
    rocksdb::DB* db{nullptr};
    options.create_if_missing=true;
    status = rocksdb::DB::Open(options,dbPath,&db);
    if (!status.ok())
    {
        BOOST_TEST_MESSAGE(fmt::format("Failed to open database: {}",status.ToString()));
    }
    BOOST_REQUIRE(status.ok());

    // done
    BOOST_TEST_MESSAGE("Database opened");
    return db;
}

void closeDatabase(rocksdb::DB* db)
{
    auto opt = rocksdb::WaitForCompactOptions{};
    opt.close_db = true;
    auto status=db->WaitForCompact(opt);
    if (!status.ok())
    {
        BOOST_TEST_MESSAGE(status.ToString());
    }
    else
    {
        BOOST_TEST_MESSAGE("Database closed");
    }

    delete db;
}

} // anonymous namespace

#endif

BOOST_AUTO_TEST_SUITE(RocksdbOp)

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

BOOST_AUTO_TEST_CASE(RocksdDbIterator)
{
    std::string dbPath=hatn::test::MultiThreadFixture::tmpFilePath("rocksdbops");

    // open
    auto db=openDatabase(dbPath);

    // put values
    rocksdb::WriteOptions writeOptions;
    FmtAllocatedBufferChar buf;
    auto putValue=[&db,&writeOptions,&buf](auto&& key, const std::string& value="hello")
    {
        buf.clear();
        fieldToStringBuf(buf,key);

        lib::string_view str{buf.data(),buf.size()};
        BOOST_TEST_MESSAGE(fmt::format("Put value {}: serialized as {}",key,str));

        buf.append(lib::string_view{"0"});
        rocksdb::Slice k{buf.data(),buf.size()};
        auto status=db->Put(writeOptions,k,value);
        if (!status.ok())
        {
            BOOST_TEST_MESSAGE(fmt::format("failed to put key {}: {}",key,status.ToString()));
        }
        BOOST_REQUIRE(status.ok());
    };

    putValue(int8_t(5));
    putValue(int16_t(2));
    putValue(int32_t(7));
    putValue(int64_t(-1));
    putValue(uint8_t(1));
    putValue(uint16_t(3));
    putValue(uint32_t(4));
    putValue(uint64_t(6));
    putValue(int8_t(-7));
    putValue(int16_t(-2));
    putValue(int32_t(-3));
    putValue(int64_t(-4));
    putValue(-5);
    putValue(-6);
    putValue(uint64_t(0));

    // iterate
    BOOST_TEST_MESSAGE("iterate");
    int i=-7;
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        BOOST_TEST_MESSAGE(fmt::format("{}: {}",i,it->key().ToString()));
        i++;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // iterate backward
    i=7;
    BOOST_TEST_MESSAGE("iterate backward");
    it = db->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToLast(); it->Valid(); it->Prev())
    {
        BOOST_TEST_MESSAGE(fmt::format("{}: {}",i,it->key().ToString()));
        i--;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // iterate with inclusive boundaries
    std::string kls;
    fieldToStringBuf(kls,-3);
    kls+="0";
    rocksdb::Slice kl{kls};
    std::string kus;
    fieldToStringBuf(kus,3);
    kus+="1";
    rocksdb::Slice ku{kus};
    rocksdb::ReadOptions readOpts;
    readOpts.iterate_lower_bound=&kl;
    readOpts.iterate_upper_bound=&ku;

    // direct order
    BOOST_TEST_MESSAGE("iterate with inclusive boundaries");
    it = db->NewIterator(readOpts);
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        BOOST_TEST_MESSAGE(it->key().ToString());;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // reverse order
    BOOST_TEST_MESSAGE("iterate backward with inclusive boundaries");
    it = db->NewIterator(readOpts);
    for (it->SeekToLast(); it->Valid(); it->Prev())
    {
        BOOST_TEST_MESSAGE(it->key().ToString());;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // iterate with exclusive boundaries
    std::string kls1;
    fieldToStringBuf(kls1,-3);
    kls1+="1";
    rocksdb::Slice kl1{kls1};
    std::string kus1;
    fieldToStringBuf(kus1,3);
    kus1+="0";
    rocksdb::Slice ku1{kus1};
    readOpts.iterate_lower_bound=&kl1;
    readOpts.iterate_upper_bound=&ku1;

    // direct order
    BOOST_TEST_MESSAGE("iterate with exclusive boundaries");
    it = db->NewIterator(readOpts);
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        BOOST_TEST_MESSAGE(it->key().ToString());;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // reverse order
    BOOST_TEST_MESSAGE("iterate backward with exclusive boundaries");
    it = db->NewIterator(readOpts);
    for (it->SeekToLast(); it->Valid(); it->Prev())
    {
        BOOST_TEST_MESSAGE(it->key().ToString());;
    }
    if (!it->status().ok())
    {
        BOOST_TEST_MESSAGE(it->status().ToString());
    }
    BOOST_REQUIRE(it->status().ok());
    delete it;

    // test other types
    putValue(true);
    putValue(false);
    putValue(std::string("how are you"));
    putValue(common::DateTime::currentUtc());
    // putValue(common::Date::currentUtc());
    // putValue(common::Time::currentUtc());
    // putValue(common::DateRange{common::Date::currentUtc()});
    // putValue(db::ObjectId::generateId());

    // close
    closeDatabase(db);
}

#else

BOOST_AUTO_TEST_CASE(RocksdDbSkip)
{
    BOOST_CHECK(true);
}

#endif

BOOST_AUTO_TEST_SUITE_END()
