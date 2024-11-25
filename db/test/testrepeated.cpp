/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/testrepeated.cpp
*/

/****************************************************************************/

#include <cmath>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

#include <hatn/base/configtreeloader.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/db/schema.h>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "modelsrep.h"

using ExtSetter=void;

#include "finddefs.h"
#include "findhandlers.ipp"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

HATN_USING
HATN_DATAUNIT_USING
HATN_DB_USING
HATN_TEST_USING

namespace {

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(TestRepeated, *boost::unit_test::fixture<HATN_TEST_NAMESPACE::DbTestFixture>())

BOOST_AUTO_TEST_CASE(CreateFindUpdate)
{
    std::ignore=genEnum;
    std::ignore=genBool;
    std::ignore=genInt8;
    std::ignore=genInt16;
    std::ignore=genInt32;
    std::ignore=genInt64;
    std::ignore=genUInt8;
    std::ignore=genUInt16;
    std::ignore=genUInt64;
    std::ignore=IntervalWidth;
    std::ignore=VectorStep;
    std::ignore=VectorSize;

    init();
    registerModels();
    auto s1=initSchema(modelRep());

    auto handler=[&s1](std::shared_ptr<DbPlugin>& plugin, std::shared_ptr<Client> client)
    {
        setSchemaToClient(client,s1);

        Topic topic1{"topic1"};
        size_t objectCount=10;
        size_t arraySize=5;

        auto run=[&](const auto& fld,const auto& gen,const auto& idx)
        {
            std::vector<ObjectId> oids;
            auto path=du::path(fld);

            // create objects
            for (size_t j=0;j<objectCount;j++)
            {
                auto o=makeInitObject<rep::type>();
                oids.emplace_back(o.fieldValue(object::_id));
                for (size_t i=0;i<arraySize;i++)
                {
                    o.fieldAtPath(path).appendValue(gen(i+j*arraySize,false));
                }

                // create object
                auto ec=client->create(topic1,modelRep(),&o);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                BOOST_REQUIRE(!ec);
            }

            // find single object
            auto val1=gen(3+4*arraySize,true);
            auto q1=makeQuery(idx,query::where(field(path),query::Operator::eq,val1),topic1);
            auto r1=client->find(modelRep(),q1);
            BOOST_REQUIRE(!r1);
            if (r1->size()==0)
            {
                BOOST_TEST_MESSAGE(fmt::format("Not found {}",val1));
            }
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found {}:\n {}",val1,r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[4]);

            // find multiple objects
            auto q2=makeQuery(idx,query::where(field(path),query::Operator::gte,val1),topic1);
            auto r2=client->find(modelRep(),q2);
            BOOST_REQUIRE(!r2);
            BOOST_REQUIRE_EQUAL(r2->size(),6);
            BOOST_CHECK(r2->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[4]);
            BOOST_CHECK(r2->at(1).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK(r2->at(2).template as<rep::type>()->fieldValue(object::_id)==oids[6]);
            BOOST_CHECK(r2->at(3).template as<rep::type>()->fieldValue(object::_id)==oids[7]);
            BOOST_CHECK(r2->at(4).template as<rep::type>()->fieldValue(object::_id)==oids[8]);
            BOOST_CHECK(r2->at(5).template as<rep::type>()->fieldValue(object::_id)==oids[9]);

            // delete object
            auto r3=client->deleteMany(modelRep(),q1);
            BOOST_REQUIRE(!r3);

            // check after delete
            r1=client->find(modelRep(),q1);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);
            r2=client->find(modelRep(),q2);
            BOOST_REQUIRE(!r2);
            BOOST_REQUIRE_EQUAL(r2->size(),5);
            BOOST_CHECK(r2->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK(r2->at(1).template as<rep::type>()->fieldValue(object::_id)==oids[6]);
            BOOST_CHECK(r2->at(2).template as<rep::type>()->fieldValue(object::_id)==oids[7]);
            BOOST_CHECK(r2->at(3).template as<rep::type>()->fieldValue(object::_id)==oids[8]);
            BOOST_CHECK(r2->at(4).template as<rep::type>()->fieldValue(object::_id)==oids[9]);

            // push to array
            size_t pushVal=10000;
            auto pushReq=update::request(
                update::field(update::path(path),update::push,pushVal)
                );
            auto val2=gen(3+5*arraySize,true);
            auto q3=makeQuery(idx,query::where(field(path),query::Operator::eq,val2),topic1);
            auto r4=client->updateMany(modelRep(),q3,pushReq);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after push:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize+1);
            auto q4=makeQuery(idx,query::where(field(path),query::Operator::eq,pushVal),topic1);
            r1=client->find(modelRep(),q4);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);

            // pop from array
            auto popReq=update::request(
                update::field(update::path(path),update::pop)
                );
            auto r5=client->updateMany(modelRep(),q3,popReq);
            BOOST_REQUIRE(!r5);
            BOOST_REQUIRE_EQUAL(r5.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after pop:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            r1=client->find(modelRep(),q4);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);

            // check before replace element
            size_t replaceIdx=1;
            size_t replaceInt=70000;
            auto replaceVal=gen(replaceInt,false);
            auto val3=gen(replaceIdx+5*arraySize,true);
            auto q5=makeQuery(idx,query::where(field(path),query::Operator::eq,val3),topic1);
            r1=client->find(modelRep(),q5);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            auto q6=makeQuery(idx,query::where(field(path),query::Operator::eq,replaceVal),topic1);
            r1=client->find(modelRep(),q6);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);

            // replace element in array
            auto replaceReq=update::request(
                update::field(update::path(array(fld,replaceIdx)),update::replace_element,replaceVal)
                );
            r4=client->updateMany(modelRep(),q3,replaceReq);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after replace:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            r1=client->find(modelRep(),q5);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);
            r1=client->find(modelRep(),q6);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            const auto& replF=r1->at(0).template as<rep::type>()->fieldAtPath(path);
            BOOST_CHECK(replF.arrayEquals(replaceIdx,replaceVal));

            // increment element in array
            auto incIdx=replaceIdx;
            size_t incVal=9;
            auto incrementedVal=gen(replaceInt+incVal,true);
            auto q7=makeQuery(idx,query::where(field(path),query::Operator::eq,incrementedVal),topic1);
            auto incReq=update::request(
                update::field(update::path(array(fld,incIdx)),update::inc_element,incVal)
                );
            r4=client->updateMany(modelRep(),q3,incReq);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after increment:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            r1=client->find(modelRep(),q6);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);
            r1=client->find(modelRep(),q7);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            const auto& incF=r1->at(0).template as<rep::type>()->fieldAtPath(path);
            BOOST_CHECK(incF.arrayEquals(incIdx,gen(replaceInt+incVal,true)));

            // erase element in array
            auto eraseReq=update::request(
                update::field(update::path(array(fld,incIdx)),update::erase_element)
                );
            r4=client->updateMany(modelRep(),q3,eraseReq);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after erase element:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize-1);
            r1=client->find(modelRep(),q7);
            BOOST_REQUIRE(!r1);
            BOOST_CHECK_EQUAL(r1->size(),0);

            // push unique element to array
            // exists, no change
            auto val4=gen(4+5*arraySize,true);
            auto pushUniqueReq1=update::request(
                update::field(update::path(path),update::push_unique,val4)
                );
            r4=client->updateMany(modelRep(),q3,pushUniqueReq1);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after try push unique element:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize-1);
            // not exist, push
            auto val5=gen(80000,true);
            auto pushUniqueReq2=update::request(
                update::field(update::path(path),update::push_unique, val5)
                );
            r4=client->updateMany(modelRep(),q3,pushUniqueReq2);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after push unique element:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            auto q8=makeQuery(idx,query::where(field(path),query::Operator::eq,val5),topic1);
            r1=client->find(modelRep(),q8);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            BOOST_CHECK_EQUAL(r1->at(0).template as<rep::type>()->sizeAtPath(path),arraySize);
            const auto& pushF=r1->at(0).template as<rep::type>()->fieldAtPath(path);
            BOOST_CHECK(pushF.arrayEquals(arraySize-1,val5));

            // set repeated field
            std::vector<decltype(gen(0,false))> vec1{gen(50100,false),gen(50200,false),gen(50300,false),
                gen(50400,false),gen(50500,false),gen(50600,false),
                gen(50700,false)
            };
            auto setReq=update::request(
                update::field(update::path(path),update::set,vec1)
                );
            r4=client->updateMany(modelRep(),q3,setReq);
            BOOST_REQUIRE(!r4);
            BOOST_REQUIRE_EQUAL(r4.value(),1);
            r1=client->find(modelRep(),q3);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),0);
            auto fixedVal=gen(50400,true);
            auto q9=makeQuery(idx,query::where(field(path),query::Operator::eq,fixedVal),topic1);
            r1=client->find(modelRep(),q9);
            BOOST_REQUIRE(!r1);
            BOOST_REQUIRE_EQUAL(r1->size(),1);
            BOOST_TEST_MESSAGE(fmt::format("Found after set vector:\n {}",r1->at(0).template as<rep::type>()->toString(true)));
            BOOST_CHECK(r1->at(0).template as<rep::type>()->fieldValue(object::_id)==oids[5]);
            const auto& setF=r1->at(0).template as<rep::type>()->fieldAtPath(path);
            BOOST_REQUIRE_EQUAL(setF.count(),vec1.size());
            for (size_t i=0;i<vec1.size();i++)
            {
                BOOST_CHECK(setF.arrayEquals(i,vec1[i]));
            }
        };

        // run(FieldUInt32,genUInt32,IdxUInt32);
        run(FieldString,genString,IdxString);
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_SUITE_END()
