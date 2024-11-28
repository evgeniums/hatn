/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testupdatenested.cpp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/db/schema.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

namespace tt = boost::test_tools;

#include "findembedded.h"
#include "updatescalarops.ipp"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
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

BOOST_AUTO_TEST_SUITE(TestUpdateNested)

BOOST_AUTO_TEST_CASE(SetSingle)
{
    setSingle<embed::type>();
}

BOOST_AUTO_TEST_CASE(UnsetSingle)
{
    unsetSingle<embed::type>();
}

BOOST_AUTO_TEST_CASE(IncSingle)
{
    incSingle<embed::type>();
}

BOOST_AUTO_TEST_CASE(MultipleFields)
{
    auto p=du::path(FieldUInt8);
    auto ts=vld::make_cref_tuple(p);
    const auto& arg0=hana::front(ts).get();
    static_assert(hana::is_a<vld::member_tag,decltype(arg0)>,"");
    auto&& val=vld::unwrap_object(vld::extract_ref(hana::front(arg0.path())));
    using type=std::decay_t<decltype(val)>;
    static_assert(hana::is_a<du::FieldTag,type>,"");

    multipleFields<embed::type>();
}

BOOST_AUTO_TEST_CASE(Bytes)
{
    testBytes<embed::type>();
}

BOOST_FIXTURE_TEST_CASE(CheckIndexes, DbTestFixture)
{
//! @todo Cleanup
#if 0
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>()));
    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<HATN_LOGCONTEXT_NAMESPACE::ContextWrapper>();
    ctx->beforeThreadProcessing();
#endif
    HATN_CTX_SCOPE("CheckIndexes")

    init();

    auto s1=initSchema(modelEmbed());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);
        checkIndexes<embed::type>(modelEmbed(),client);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");

//! @todo Cleanup
#if 0
    ctx->afterThreadProcessing();
#endif
}

BOOST_AUTO_TEST_SUITE_END()
