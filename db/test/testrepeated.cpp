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
#include <hatn/db/update.h>
#include <hatn/db/ipp/updateunit.ipp>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

#include "modelsrep.h"

using ExtSetter=void;

#include "finddefs.h"
#include "findhandlers.ipp"

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>
#endif

namespace tt = boost::test_tools;

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

    auto handler=[&s1](std::shared_ptr<DbPlugin> plugin, std::shared_ptr<Client> client)
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

                auto& f=o.mutableField(fld);
                for (size_t i=0;i<arraySize;i++)
                {
                    auto val=gen(i+j*arraySize,false);
                    f.appendValue(val);
                }

                // create object
                auto ec=client->create(topic1,modelRep(),&o);
                if (ec)
                {
                    BOOST_TEST_MESSAGE(ec.message());
                }
                BOOST_REQUIRE(!ec);

                std::cout << "Added " << j << ": " << o.toString(true) << std::endl;

                oids.emplace_back(o.fieldValue(object::_id));
            }

            // find single object
            auto val1=gen(3+4*arraySize,true);
            std::cout << "val1=" << val1 << std::endl;
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
            BOOST_TEST_MESSAGE(fmt::format("Found gte {}:",val1));
            for (size_t i=0;i<r2->size();i++)
            {
                std::cout << i << ": " << r2->at(i).template as<rep::type>()->toString(true) << std::endl;
            }
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
            auto pushVal=gen(10000,false);
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
            if constexpr (std::is_arithmetic<decltype(gen(0,false))>::value)
            {
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
            }

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
            if constexpr (std::is_arithmetic<decltype(gen(0,false))>::value)
            {
                r1=client->find(modelRep(),q7);
                BOOST_REQUIRE(!r1);
                BOOST_CHECK_EQUAL(r1->size(),0);
            }

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

        BOOST_TEST_CONTEXT("FieldUInt32"){run(FieldUInt32,genUInt32,IdxUInt32);}
        BOOST_TEST_CONTEXT("FieldString"){run(FieldString,genString,IdxString);}
    };
    PrepareDbAndRun::eachPlugin(handler,"simple1.jsonc");
}

BOOST_AUTO_TEST_CASE(UpdateFloatingPoint)
{
    auto run=[](auto sample, const auto& path, const auto& field)
    {
        auto o=makeInitObject<rep::type>();
        using type=decltype(sample);

        // set vector
        std::vector<type> vec1{type(100.01),type(200.02),type(300.03),type(400.004),type(500.005)};
        auto req1=update::request(
            update::field(update::path(path),update::set,vec1)
        );
        auto ec=update::apply(&o,req1);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_TEST(o.fieldAtPath(path).at(i)==vec1[i], tt::tolerance(0.0001));
        }

        // push
        auto val2=type(1000.505);
        auto req2=update::request(
            update::field(update::path(path),update::push,val2)
        );
        ec=update::apply(&o,req2);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size()+1);
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_TEST(o.fieldAtPath(path).at(i)==vec1[i], tt::tolerance(0.0001));
        }
        BOOST_TEST(o.fieldAtPath(path).at(vec1.size())==val2, tt::tolerance(0.0001));

        // pop
        auto req3=update::request(
                update::field(update::path(path),update::pop)
            );
        ec=update::apply(&o,req3);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_TEST(o.fieldAtPath(path).at(i)==vec1[i], tt::tolerance(0.0001));
        }

        // increment element
        auto inc4=type(15.709);
        size_t incIdx=3;
        auto req4=update::request(
                update::field(update::path(array(field,incIdx)),update::inc_element,inc4)
            );
        ec=update::apply(&o,req4);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            if (i==incIdx)
            {
                BOOST_TEST(o.fieldAtPath(path).at(i)==(vec1[i]+inc4), tt::tolerance(0.0001));
            }
            else
            {
                BOOST_TEST(o.fieldAtPath(path).at(i)==vec1[i], tt::tolerance(0.0001));
            }
        }

        // replace element
        auto repl5=type(20097.831);
        size_t replIdx=2;
        auto req5=update::request(
            update::field(update::path(array(field,replIdx)),update::replace_element,repl5)
            );
        ec=update::apply(&o,req5);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            if (i==incIdx)
            {
                BOOST_TEST(o.fieldAtPath(path).at(i)==(vec1[i]+inc4), tt::tolerance(0.0001));
            }
            else if (i==replIdx)
            {
                BOOST_TEST(o.fieldAtPath(path).at(i)==repl5, tt::tolerance(0.0001));
            }
            else
            {
                BOOST_TEST(o.fieldAtPath(path).at(i)==vec1[i], tt::tolerance(0.0001));
            }
        }

        // erase element
        size_t eraseIdx=1;
        auto req6=update::request(
            update::field(update::path(array(field,eraseIdx)),update::erase_element)
            );
        ec=update::apply(&o,req6);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size()-1);
        for (size_t i=0;i<vec1.size();i++)
        {
            size_t j=i;
            if (i==eraseIdx)
            {
                i++;
            }
            if (i>eraseIdx)
            {
                j=i-1;
            }

            if (i==incIdx)
            {
                BOOST_TEST(o.fieldAtPath(path).at(j)==(vec1[i]+inc4), tt::tolerance(0.0001));
            }
            else if (i==replIdx)
            {
                BOOST_TEST(o.fieldAtPath(path).at(j)==repl5, tt::tolerance(0.0001));
            }
            else
            {
                BOOST_TEST(o.fieldAtPath(path).at(j)==vec1[i], tt::tolerance(0.0001));
            }
        }
    };

    run(float(0),du::path(FieldFloat),FieldFloat);
    run(double(0),du::path(FieldDouble),FieldDouble);
}

BOOST_AUTO_TEST_CASE(UpdateBytes)
{
    auto run=[](const auto& path, const auto& field)
    {
        auto o=makeInitObject<rep::type>();

        // set vector
        //! @todo Support vector of vector<char>?
        std::vector<std::string> vec1{"one","two","three","four", "five"};
        auto req1=update::request(
            update::field(update::path(path),update::set,vec1)
            );
        auto ec=update::apply(&o,req1);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),vec1[i]);
        }

        // push
        std::vector<char> val2{'v','e','c','t','o','r','2'};
        auto req2=update::request(
            update::field(update::path(path),update::push,val2)
            );
        ec=update::apply(&o,req2);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size()+1);
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),vec1[i]);
        }
        auto res2=lib::string_view{o.fieldAtPath(path).at(vec1.size()).dataPtr(),o.fieldAtPath(path).at(vec1.size()).dataSize()};
        auto check2=lib::string_view{val2.data(),val2.size()};
        BOOST_CHECK(res2==check2);

        // pop
        auto req3=update::request(
            update::field(update::path(path),update::pop)
            );
        ec=update::apply(&o,req3);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),vec1[i]);
        }

        // replace element
        std::vector<char> repl5{'v','e','c','t','o','r','3'};
        auto repl5Check=lib::string_view{repl5.data(),repl5.size()};
        size_t replIdx=2;
        auto req5=update::request(
            update::field(update::path(array(field,replIdx)),update::replace_element,repl5)
            );
        ec=update::apply(&o,req5);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size());
        for (size_t i=0;i<vec1.size();i++)
        {
            if (i==replIdx)
            {
                auto res=lib::string_view{o.fieldAtPath(path).at(i).dataPtr(),o.fieldAtPath(path).at(i).dataSize()};
                BOOST_CHECK(res==repl5Check);
            }
            else
            {
                BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),vec1[i]);
            }
        }

        // erase element
        size_t eraseIdx=1;
        auto req6=update::request(
            update::field(update::path(array(field,eraseIdx)),update::erase_element)
            );
        ec=update::apply(&o,req6);
        BOOST_REQUIRE(!ec);
        BOOST_CHECK(o.fieldAtPath(path).isSet());
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),vec1.size()-1);
        for (size_t i=0;i<vec1.size();i++)
        {
            size_t j=i;
            if (i==eraseIdx)
            {
                i++;
            }
            if (i>eraseIdx)
            {
                j=i-1;
            }

            if (i==replIdx)
            {
                auto res=lib::string_view{o.fieldAtPath(path).at(j).dataPtr(),o.fieldAtPath(path).at(j).dataSize()};
                BOOST_CHECK(res==repl5Check);
            }
            else
            {
                BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(j).c_str(),vec1[i]);
            }
        }
    };

    run(du::path(FieldBytes),FieldBytes);
}

#ifdef HATN_DB_VALUE_COPY

// update requests must not refer to the data they were built from
BOOST_AUTO_TEST_CASE(UpdateRequestOwnsValues)
{
    const auto path=du::path(FieldBytes);

    // values are longer than SSO limit to make sure that they are allocated on the heap
    const std::vector<std::string> setValues{
        "set-value-0-longer-than-the-small-string-limit",
        "set-value-1-longer-than-the-small-string-limit",
        "set-value-2-longer-than-the-small-string-limit"
    };
    const std::string pushLvalue{"push-lvalue-longer-than-the-small-string-limit"};
    const std::string pushRvalue{"push-rvalue-longer-than-the-small-string-limit"};
    const std::string pushView{"push-view-longer-than-the-small-string-limit"};
    const std::string pushBytes{"push-bytes-longer-than-the-small-string-limit"};
    const std::vector<std::string> resetValues{
        "reset-value-0-longer-than-the-small-string-limit",
        "reset-value-1-longer-than-the-small-string-limit"
    };

    // sources are overwritten and freed before the requests are returned
    auto build=[&]()
    {
        std::vector<common::SharedPtr<update::Request>> requests;

        auto setSrc=std::make_unique<std::vector<std::string>>(setValues);
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::set,*setSrc)
        ));

        auto pushSrc=std::make_unique<std::string>(pushLvalue);
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::push,*pushSrc)
        ));

        // temporary string
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::push,std::string{pushRvalue})
        ));

        auto viewSrc=std::make_unique<std::string>(pushView);
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::push,lib::string_view{viewSrc->data(),viewSrc->size()})
        ));

        auto bytesSrc=std::make_unique<std::vector<char>>(pushBytes.begin(),pushBytes.end());
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::push,*bytesSrc)
        ));

        // temporary vector
        requests.push_back(update::sharedRequest(
            update::field(update::path(path),update::set,std::vector<std::string>{resetValues})
        ));

        for (auto& it: *setSrc)
        {
            it.assign(it.size(),'#');
        }
        pushSrc->assign(pushSrc->size(),'#');
        viewSrc->assign(viewSrc->size(),'#');
        bytesSrc->assign(bytesSrc->size(),'#');

        return requests;
    };
    auto requests=build();
    BOOST_REQUIRE_EQUAL(requests.size(),6);

    auto o=makeInitObject<rep::type>();

    // set vector
    auto ec=update::apply(&o,*requests[0]);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),setValues.size());
    for (size_t i=0;i<setValues.size();i++)
    {
        BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),setValues[i]);
    }

    // push lvalue string, temporary string, string_view and bytes
    const std::vector<std::string> pushed{pushLvalue,pushRvalue,pushView,pushBytes};
    for (size_t i=0;i<pushed.size();i++)
    {
        ec=update::apply(&o,*requests[i+1]);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),setValues.size()+i+1);
        const auto& item=o.fieldAtPath(path).at(setValues.size()+i);
        auto res=lib::string_view{item.dataPtr(),item.dataSize()};
        auto check=lib::string_view{pushed[i]};
        BOOST_CHECK(res==check);
    }
    for (size_t i=0;i<setValues.size();i++)
    {
        BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),setValues[i]);
    }

    // set temporary vector
    ec=update::apply(&o,*requests[5]);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(o.fieldAtPath(path).count(),resetValues.size());
    for (size_t i=0;i<resetValues.size();i++)
    {
        BOOST_CHECK_EQUAL(o.fieldAtPath(path).at(i).c_str(),resetValues[i]);
    }
}

// queries must not refer to the data they were built from
BOOST_AUTO_TEST_CASE(QueryOwnsValues)
{
    const std::string topic{"query-owns-values-topic"};
    const std::vector<std::string> inValues{
        "in-value-0-longer-than-the-small-string-limit",
        "in-value-1-longer-than-the-small-string-limit",
        "in-value-2-longer-than-the-small-string-limit"
    };
    const std::string eqValue{"eq-value-longer-than-the-small-string-limit"};

    auto collect=[](const auto& query)
    {
        std::vector<std::string> items;
        auto ec=query.field(0).value.eachVectorItem(
            [&items](const auto& item)
            {
                // the visitor is instantiated for all vector types, only strings are expected here
                if constexpr (std::is_same<std::decay_t<decltype(item)>,std::string>::value)
                {
                    items.emplace_back(item.data(),item.size());
                }
                return Error{};
            }
        );
        BOOST_REQUIRE(!ec);
        return items;
    };

    // lvalue vector
    {
        auto buildIn=[&]()
        {
            auto src=std::make_unique<std::vector<std::string>>(inValues);
            auto q=makeQuery(rep_f10_idx(),query::where(rep::f10,query::Operator::in,*src),topic);
            for (auto& it: *src)
            {
                it.assign(it.size(),'#');
            }
            return q;
        };
        auto q=buildIn();
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        BOOST_CHECK(collect(q)==inValues);
    }

    // temporary vector
    {
        auto q=makeQuery(rep_f10_idx(),query::where(rep::f10,query::Operator::in,std::vector<std::string>{inValues}),topic);
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        BOOST_CHECK(collect(q)==inValues);
    }

    // lvalue string
    {
        auto buildEq=[&]()
        {
            auto src=std::make_unique<std::string>(eqValue);
            auto q=makeQuery(rep_f10_idx(),query::where(rep::f10,query::Operator::eq,*src),topic);
            src->assign(src->size(),'#');
            return q;
        };
        auto q=buildEq();
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        const auto& val=q.field(0).value.as<query::String>();
        BOOST_CHECK(val==lib::string_view{eqValue});
    }

    // temporary string
    {
        auto q=makeQuery(rep_f10_idx(),query::where(rep::f10,query::Operator::eq,std::string{eqValue}),topic);
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        const auto& val=q.field(0).value.as<query::String>();
        BOOST_CHECK(val==lib::string_view{eqValue});
    }

    // string_view
    {
        auto buildView=[&]()
        {
            auto src=std::make_unique<std::string>(eqValue);
            auto q=makeQuery(rep_f10_idx(),query::where(rep::f10,query::Operator::eq,lib::string_view{src->data(),src->size()}),topic);
            src->assign(src->size(),'#');
            return q;
        };
        auto q=buildView();
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        const auto& val=q.field(0).value.as<query::String>();
        BOOST_CHECK(val==lib::string_view{eqValue});
    }

    // where object that is stored and used later
    {
        auto buildWhere=[&]()
        {
            auto src=std::make_unique<std::string>(eqValue);
            auto w=query::where(rep::f10,query::Operator::eq,*src);
            src->assign(src->size(),'#');
            return w;
        };
        auto w=buildWhere();
        auto q=makeQuery(rep_f10_idx(),w,topic);
        BOOST_REQUIRE_EQUAL(q.fields().size(),1);
        const auto& val=q.field(0).value.as<query::String>();
        BOOST_CHECK(val==lib::string_view{eqValue});
    }
}

#endif

BOOST_AUTO_TEST_SUITE_END()
