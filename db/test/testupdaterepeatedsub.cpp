/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testupdaterepeatedsub.cpp
*
*   Tests for update operations on fields nested inside repeated-subunit
*   elements (db::update() previously supported only depth-1 array element
*   operators; see db/update.h and db/ipp/updateunit.ipp).
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>
#include <hatn/common/makeshared.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/db/schema.h>
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>
#include <hatn/db/ipp/updateserialization.ipp>

#include <hatn/dataunit/syntax.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/db/object.h>
#include <hatn/db/model.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING
HATN_LOGCONTEXT_USING

namespace {

// A subunit nested two levels deep: repsub -> items[i] -> leaf, and also
// repsub -> items[i] -> leaves[j], so it doubles as both a plain-subunit and
// a repeated-subunit leaf.
HDU_UNIT(repsub_leaf,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_REPEATED_FIELD(tags,TYPE_STRING,2)
)

// The repeated-subunit element type of repsub::items, also reused as-is for
// repsub::sub (a plain, non-repeated subunit field of the same type) so that
// "array -> ... " and "plain subunit -> ..." traversal share exactly the
// same nested shape.
HDU_UNIT(repsub_item,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(state,TYPE_UINT32,2)
    HDU_FIELD(when,TYPE_DATETIME,3)
    HDU_FIELD(leaf,repsub_leaf::TYPE,4)
    HDU_REPEATED_FIELD(nums,TYPE_INT32,5)
    HDU_REPEATED_FIELD(leaves,repsub_leaf::TYPE,6)
)

HDU_UNIT_WITH(repsub,(HDU_BASE(object)),
    HDU_FIELD(f1,TYPE_UINT32,1)
    HDU_FIELD(marker,TYPE_STRING,2)
    HDU_REPEATED_FIELD(items,repsub_item::TYPE,3)
    HDU_FIELD(sub,repsub_item::TYPE,4)
)

HATN_DB_INDEX(repsub_f1_idx,repsub::f1)
HATN_DB_INDEX(repsub_marker_idx,repsub::marker)

HATN_DB_MODEL(modelRepSub,repsub,repsub_f1_idx(),repsub_marker_idx())

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
namespace rdb=HATN_ROCKSDB_NAMESPACE;
#endif

void registerModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    rdb::RocksdbModels::instance().registerModel(modelRepSub());
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
    auto schema1=makeSchema("schema_updaterepeatedsub",std::forward<Models>(models)...);

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

// Populates item/sub with deterministic, reproducible content so that two
// separately constructed items with the same index and the same `when` are
// byte-for-byte identical (needed by the push_unique-on-subunit test, which
// checks that an exact duplicate is *not* appended). Works on any type that
// exposes the standard field API: the array element wrapper returned by
// appendSharedSubunit()/at(), the plain-subunit field wrapper returned by
// (mutable)field(repsub::sub), and a standalone managed unit alike.
template <typename ItemT>
void populateRepSubItem(ItemT& item, size_t i, const common::DateTime& when)
{
    item.setFieldValue(repsub_item::name,fmt::format("item{}",i));
    item.setFieldValue(repsub_item::state,static_cast<uint32_t>(i));
    item.setFieldValue(repsub_item::when,when);
    item.mutableField(repsub_item::leaf).setFieldValue(repsub_leaf::name,fmt::format("leaf{}",i));

    auto& nums=item.mutableField(repsub_item::nums);
    for (int32_t n=0;n<3;n++)
    {
        nums.appendValue(static_cast<int32_t>(i*10+n));
    }

    auto& leaves=item.mutableField(repsub_item::leaves);
    for (size_t j=0;j<2;j++)
    {
        auto leafEl=leaves.appendSharedSubunit();
        leafEl.setFieldValue(repsub_leaf::name,fmt::format("item{}_leaf{}",i,j));
    }
}

auto makeRepSubObject(uint32_t f1val, const std::string& marker, const common::DateTime& when, size_t itemCount=3)
{
    auto o=makeInitObject<repsub::type>();
    o.setFieldValue(repsub::f1,f1val);
    o.setFieldValue(repsub::marker,marker);

    for (size_t i=0;i<itemCount;i++)
    {
        auto item=o.mutableField(repsub::items).appendSharedSubunit();
        populateRepSubItem(item,i,when);
    }

    auto& subItem=o.mutableField(repsub::sub);
    populateRepSubItem(subItem,100,when);

    return o;
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestUpdateRepeatedSub)

BOOST_AUTO_TEST_CASE(NestedScalar)
{
    auto when=common::DateTime::currentUtc();
    auto obj=makeRepSubObject(1,"m1",when);

    // set items[1].state
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),repsub_item::state),update::set,uint32_t(777))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).fieldValue(repsub_item::state),777u);
        // siblings untouched
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(0).fieldValue(repsub_item::state),0u);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(2).fieldValue(repsub_item::state),2u);
    }

    // unset items[0].name
    {
        BOOST_CHECK(obj.field(repsub::items).at(0).isSet(repsub_item::name));
        auto req=update::request(
            update::field(update::path(array(repsub::items,0),repsub_item::name),update::unset)
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(!obj.field(repsub::items).at(0).isSet(repsub_item::name));
    }

    // inc items[2].state
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,2),repsub_item::state),update::inc,int32_t(5))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(2).fieldValue(repsub_item::state),7u);
    }

    // set items[0].when
    {
        auto newWhen=common::DateTime::currentUtc();
        auto req=update::request(
            update::field(update::path(array(repsub::items,0),repsub_item::when),update::set,newWhen)
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(obj.field(repsub::items).at(0).fieldValue(repsub_item::when)==newWhen);
    }
}

BOOST_AUTO_TEST_CASE(DeepNesting)
{
    auto when=common::DateTime::currentUtc();
    auto obj=makeRepSubObject(1,"m1",when);

    // items[1].leaf.name : array -> plain subunit -> scalar
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),repsub_item::leaf,repsub_leaf::name),update::set,"changed-leaf")
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::leaf).fieldValue(repsub_leaf::name),std::string("changed-leaf"));
    }

    // items[1].leaves[0].name : array -> repeated subunit -> scalar
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),array(repsub_item::leaves,0),repsub_leaf::name),update::set,"changed-nested-leaf")
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::leaves).at(0).fieldValue(repsub_leaf::name),std::string("changed-nested-leaf"));
        // sibling leaf element untouched
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::leaves).at(1).fieldValue(repsub_leaf::name),std::string("item1_leaf1"));
    }

    // sub.nums[1] : plain subunit -> repeated scalar, element op
    {
        auto req=update::request(
            update::field(update::path(repsub::sub,array(repsub_item::nums,1)),update::replace_element,int32_t(-500))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::sub).field(repsub_item::nums).at(1),-500);
        // sibling element untouched
        BOOST_CHECK_EQUAL(obj.field(repsub::sub).field(repsub_item::nums).at(0),1000);
    }
}

BOOST_AUTO_TEST_CASE(ElementOpsNested)
{
    auto when=common::DateTime::currentUtc();
    auto obj=makeRepSubObject(1,"m1",when);

    // push onto items[1].nums
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),repsub_item::nums),update::push,int32_t(555))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        const auto& nums=obj.field(repsub::items).at(1).field(repsub_item::nums);
        BOOST_REQUIRE_EQUAL(nums.count(),4u);
        BOOST_CHECK_EQUAL(nums.at(3),555);
    }

    // replace_element on items[1].nums[0]
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),array(repsub_item::nums,0)),update::replace_element,int32_t(-1))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::nums).at(0),-1);
    }

    // inc_element on items[1].nums[1]
    {
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),array(repsub_item::nums,1)),update::inc_element,int32_t(100))
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::nums).at(1),111); // original 11 (1*10+1) + 100
    }

    // erase_element on items[1].nums[0]
    {
        auto sizeBefore=obj.field(repsub::items).at(1).field(repsub_item::nums).count();
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),array(repsub_item::nums,0)),update::erase_element)
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::nums).count(),sizeBefore-1);
    }

    // pop on items[1].nums
    {
        auto sizeBefore=obj.field(repsub::items).at(1).field(repsub_item::nums).count();
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),repsub_item::nums),update::pop)
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).field(repsub_item::nums).count(),sizeBefore-1);
    }

    // push / push_unique on items[2].leaf.tags : array -> plain subunit -> repeated scalar
    {
        auto req1=update::request(
            update::field(update::path(array(repsub::items,2),repsub_item::leaf,repsub_leaf::tags),update::push,"tag-a")
            );
        auto ec=update::apply(&obj,req1);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(2).field(repsub_item::leaf).field(repsub_leaf::tags).count(),1u);

        auto req2=update::request(
            update::field(update::path(array(repsub::items,2),repsub_item::leaf,repsub_leaf::tags),update::push_unique,"tag-a")
            );
        ec=update::apply(&obj,req2);
        BOOST_REQUIRE(!ec);
        // duplicate: count unchanged
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(2).field(repsub_item::leaf).field(repsub_leaf::tags).count(),1u);

        auto req3=update::request(
            update::field(update::path(array(repsub::items,2),repsub_item::leaf,repsub_leaf::tags),update::push_unique,"tag-b")
            );
        ec=update::apply(&obj,req3);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(2).field(repsub_item::leaf).field(repsub_leaf::tags).count(),2u);
    }
}

BOOST_AUTO_TEST_CASE(WholeSubunit)
{
    auto when=common::DateTime::currentUtc();
    auto obj=makeRepSubObject(1,"m1",when);

    // push a whole new subunit onto repsub::items
    {
        auto prevCount=obj.field(repsub::items).count();

        auto newItem=common::makeShared<repsub_item::managed>();
        populateRepSubItem(*newItem,99,when);

        auto req=update::request(
            update::field(update::path(repsub::items),update::push,common::SharedPtr<du::Unit>{newItem})
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(obj.field(repsub::items).count(),prevCount+1);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(prevCount).fieldValue(repsub_item::name),std::string("item99"));
    }

    // replace_element with a whole new subunit
    {
        auto newItem=common::makeShared<repsub_item::managed>();
        populateRepSubItem(*newItem,98,when);

        auto req=update::request(
            update::field(update::path(array(repsub::items,0)),update::replace_element,common::SharedPtr<du::Unit>{newItem})
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(0).fieldValue(repsub_item::name),std::string("item98"));
    }

    // push_unique: an exact duplicate of an existing element is a no-op
    {
        auto prevCount=obj.field(repsub::items).count();

        auto dupItem=common::makeShared<repsub_item::managed>();
        populateRepSubItem(*dupItem,1,when); // matches items[1] exactly (same index, same `when`)

        auto req=update::request(
            update::field(update::path(repsub::items),update::push_unique,common::SharedPtr<du::Unit>{dupItem})
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).count(),prevCount);
    }

    // push_unique: a genuinely new element gets appended
    {
        auto prevCount=obj.field(repsub::items).count();

        auto newItem=common::makeShared<repsub_item::managed>();
        populateRepSubItem(*newItem,97,when);

        auto req=update::request(
            update::field(update::path(repsub::items),update::push_unique,common::SharedPtr<du::Unit>{newItem})
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(obj.field(repsub::items).count(),prevCount+1);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(prevCount).fieldValue(repsub_item::name),std::string("item97"));
    }

    // set: replace the whole items array with a fresh vector of subunits
    {
        std::vector<common::SharedPtr<du::Unit>> newItems;
        for (size_t i=0;i<2;i++)
        {
            auto ni=common::makeShared<repsub_item::managed>();
            populateRepSubItem(*ni,200+i,when);
            newItems.push_back(common::SharedPtr<du::Unit>{ni});
        }

        auto req=update::request(
            update::field(update::path(repsub::items),update::set,newItems)
            );
        auto ec=update::apply(&obj,req);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(obj.field(repsub::items).count(),2u);
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(0).fieldValue(repsub_item::name),std::string("item200"));
        BOOST_CHECK_EQUAL(obj.field(repsub::items).at(1).fieldValue(repsub_item::name),std::string("item201"));
    }
}

BOOST_AUTO_TEST_CASE(OutOfRange)
{
    auto when=common::DateTime::currentUtc();
    auto obj=makeRepSubObject(1,"m1",when);
    auto before=obj.toString(false,4);

    // items has 3 elements (0..2): index 99 is out of range everywhere it
    // appears in a path, so every one of these requests must be a no-op.
    auto req=update::request(
        update::field(update::path(array(repsub::items,99),repsub_item::state),update::set,uint32_t(1)),
        update::field(update::path(array(repsub::items,1),array(repsub_item::nums,99)),update::replace_element,int32_t(1)),
        update::field(update::path(array(repsub::items,99)),update::erase_element)
        );
    auto ec=update::apply(&obj,req);
    BOOST_CHECK(!ec);
    BOOST_CHECK_EQUAL(before,obj.toString(false,4));
}

BOOST_AUTO_TEST_CASE(BadPath)
{
    auto when=common::DateTime::currentUtc();

    // unknown field id anywhere in the path
    {
        auto obj=makeRepSubObject(1,"m1",when);
        db::FieldPath badPath;
        badPath.emplace_back(999999,"bogus",-1);
        auto req=update::request(update::field(badPath,update::set,uint32_t(1)));
        auto ec=update::apply(&obj,req);
        BOOST_CHECK(ec);
    }

    // array field traversed as if it were a plain field (no db::array())
    {
        auto obj=makeRepSubObject(1,"m1",when);
        auto req=update::request(
            update::field(update::path(repsub::items,repsub_item::state),update::set,uint32_t(1))
            );
        auto ec=update::apply(&obj,req);
        BOOST_CHECK(ec);
    }

    // plain scalar field traversed as if it were a subunit
    {
        auto obj=makeRepSubObject(1,"m1",when);
        auto req=update::request(
            update::field(update::path(repsub::f1,repsub_item::state),update::set,uint32_t(1))
            );
        auto ec=update::apply(&obj,req);
        BOOST_CHECK(ec);
    }
}

namespace {

// Applies req to a fresh object directly, and separately to another fresh
// object after a serialize->deserialize round trip; checks both objects end
// up identical, and that re-serializing the deserialized request reproduces
// the original wire message.
void checkRoundTrip(const update::Request& req)
{
    auto when=common::DateTime::currentUtc();
    auto obj1=makeRepSubObject(1,"m1",when);
    auto obj2=makeRepSubObject(1,"m1",when);

    auto ec1=update::apply(&obj1,req);
    BOOST_REQUIRE(!ec1);

    update::message::type msg;
    auto ec2=update::serialize(req,msg);
    BOOST_REQUIRE(!ec2);

    update::Request req2;
    common::pmr::vector<update::serialization::VectorsHolder> vectorsHolder;
    auto ec3=update::deserialize(msg,req2,vectorsHolder);
    BOOST_REQUIRE(!ec3);

    auto ec4=update::apply(&obj2,req2);
    BOOST_REQUIRE(!ec4);

    // obj1/obj2 are independently constructed (each gets its own fresh _id),
    // so compare field content only, excluding _id/created_at/updated_at.
    BOOST_CHECK(du::unitsEqual(&obj1,&obj2,object::_id,object::created_at,object::updated_at));

    // re-serializing the deserialized request must reproduce the same wire form
    update::message::type msg2;
    auto ec5=update::serialize(req2,msg2);
    BOOST_REQUIRE(!ec5);
    BOOST_CHECK_EQUAL(msg.toString(false,4),msg2.toString(false,4));
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(Serialize)
{
    // nested scalar set
    checkRoundTrip(update::request(
        update::field(update::path(array(repsub::items,1),repsub_item::state),update::set,uint32_t(777))
        ));

    // deep nesting: array -> repeated subunit -> scalar
    checkRoundTrip(update::request(
        update::field(update::path(array(repsub::items,1),array(repsub_item::leaves,0),repsub_leaf::name),update::set,"changed")
        ));

    // element ops on a repeated scalar nested inside a repeated-subunit element
    checkRoundTrip(update::request(
        update::field(update::path(array(repsub::items,1),repsub_item::nums),update::push,int32_t(555)),
        update::field(update::path(array(repsub::items,1),array(repsub_item::nums,0)),update::replace_element,int32_t(-1)),
        update::field(update::path(array(repsub::items,1),array(repsub_item::nums,1)),update::inc_element,int32_t(100)),
        update::field(update::path(array(repsub::items,1),array(repsub_item::nums,0)),update::erase_element)
        ));

    // whole subunit operand: push a new element onto repsub::items
    {
        auto when=common::DateTime::currentUtc();
        auto newItem=common::makeShared<repsub_item::managed>();
        populateRepSubItem(*newItem,99,when);
        checkRoundTrip(update::request(
            update::field(update::path(repsub::items),update::push,common::SharedPtr<du::Unit>{newItem})
            ));
    }
}

BOOST_FIXTURE_TEST_CASE(DbUpdate, DbTestFixture)
{
    HATN_CTX_SCOPE("DbUpdate")

    init();
    auto s1=initSchema(modelRepSub());

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};
        auto when=common::DateTime::currentUtc();
        auto obj=makeRepSubObject(555,"markerA",when);

        auto ec=client->create(topic1,modelRepSub(),&obj);
        BOOST_REQUIRE(!ec);

        auto q1=makeQuery(repsub_f1_idx(),query::where(repsub::f1,query::eq,uint32_t(555)),topic1);

        // update items[1].state through the db client
        auto req=update::request(
            update::field(update::path(array(repsub::items,1),repsub_item::state),update::set,uint32_t(4242))
            );
        auto r1=client->updateMany(modelRepSub(),q1,req);
        BOOST_REQUIRE(!r1);
        BOOST_REQUIRE_EQUAL(r1.value(),1u);

        auto r2=client->find(modelRepSub(),q1);
        BOOST_REQUIRE(!r2);
        BOOST_REQUIRE_EQUAL(r2->size(),1u);
        const auto* found=r2->at(0).unit<repsub::type>();
        BOOST_CHECK_EQUAL(found->field(repsub::items).at(1).fieldValue(repsub_item::state),4242u);
        // siblings untouched
        BOOST_CHECK_EQUAL(found->field(repsub::items).at(0).fieldValue(repsub_item::state),0u);
        BOOST_CHECK_EQUAL(found->field(repsub::items).at(2).fieldValue(repsub_item::state),2u);

        // sibling indexes (on fields untouched by the update) still resolve
        auto q2=makeQuery(repsub_marker_idx(),query::where(repsub::marker,query::eq,"markerA"),topic1);
        auto r3=client->find(modelRepSub(),q2);
        BOOST_REQUIRE(!r3);
        BOOST_REQUIRE_EQUAL(r3->size(),1u);

        // push onto a repeated scalar field nested inside a repeated-subunit element
        auto oid=found->fieldValue(object::_id);
        auto req2=update::request(
            update::field(update::path(array(repsub::items,0),repsub_item::nums),update::push,int32_t(999))
            );
        auto ec2=client->update(topic1,modelRepSub(),oid,req2);
        BOOST_REQUIRE(!ec2);

        auto r4=client->find(modelRepSub(),q1);
        BOOST_REQUIRE(!r4);
        BOOST_REQUIRE_EQUAL(r4->size(),1u);
        const auto* found2=r4->at(0).unit<repsub::type>();
        const auto& nums=found2->field(repsub::items).at(0).field(repsub_item::nums);
        BOOST_REQUIRE_EQUAL(nums.count(),4u);
        BOOST_CHECK_EQUAL(nums.at(3),999);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
