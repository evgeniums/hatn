/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/preparedb.h
 *
 *     Prepare database for tests.
 *
 */
/****************************************************************************/

#ifndef HATNPREPAREDB_H
#define HATNPREPAREDB_H

#include <vector>

#include <hatn/common/datetime.h>

#include <hatn/db/dbplugin.h>
#include <hatn/db/model.h>

#include <hatn_test_config.h>

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>
#endif

HATN_TEST_NAMESPACE_BEGIN

using TestFn=std::function<void (std::shared_ptr<db::DbPlugin>& plugin, std::shared_ptr<db::Client> client)>;

struct PartitionRange
{
    std::vector<db::ModelInfo> models;
    common::Date from;
    common::Date to;
};

struct PrepareDbAndRun
{
    static void eachPlugin(
        const TestFn& fn,
        const std::string& testConfigFile,
        const PartitionRange& partitionRange
    )
    {
        eachPlugin(fn,testConfigFile,std::vector<PartitionRange>{partitionRange});
    }

    static void eachPlugin(
        const TestFn& fn,
        const std::string& testConfigFile,
        const std::vector<PartitionRange>& partitions=std::vector<PartitionRange>{}
    );
};

HATN_TEST_NAMESPACE_END

#endif // HATNPREPAREDB_H
