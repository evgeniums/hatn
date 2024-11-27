/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/updatescalarops.ipp
*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/unit.h>

#include <hatn/db/db.h>
#include <hatn/db/topic.h>

namespace {

template <typename T>
void testUnit(const du::Unit& obj, const T& testField)
{
    BOOST_TEST_CONTEXT(testField.name())
    {
        obj.iterateFieldsConst(
            [&](const du::Field& field)
            {
                if (field.getID()!=testField.id())
                {
                    BOOST_CHECK(!field.isSet());
                }
                return true;
            }
        );
    }
}

template <typename UnitT, typename ...Path>
void checkOtherFields(
        const UnitT& o,
        Path&&... testFieldPath
    )
{
#if 0
    auto testUnit=[](const du::Unit& u, const auto& testField)
    {
        BOOST_TEST_CONTEXT(testField.name()){
            u.iterateFieldsConst(
                [&](const du::Field& field)
                {
                    if (field.getID()!=testField.id())
                    {
                        BOOST_CHECK(!field.isSet());
                    }
                    return true;
                }
            );
        }
    };
#endif

    if constexpr (sizeof...(Path)==1)
    {
        testUnit(o,std::forward<Path>(testFieldPath)...);
    }
    else
    {
        auto ts=vld::make_cref_tuple(std::forward<Path>(testFieldPath)...);
        const auto& unitField=hana::front(ts).get();
        const auto& testField=hana::back(ts).get();
        const auto& unitF=o.field(unitField);
        testUnit(*unitF.subunit(),testField);
    }
}

struct normalTypeEqT
{
    template <typename UnitT, typename T, typename ...Path>
    void operator ()(const UnitT& o, const T& val, const Path&... path) const
    {
        BOOST_CHECK_EQUAL(o.getAtPath(du::path(path...)),val);
    }
};
constexpr normalTypeEqT normalTypeEq{};

struct extraTypeEqT
{
    template <typename UnitT, typename T, typename ...Path>
    void operator ()(const UnitT& o, const T& val, const Path&... path) const
    {
        BOOST_CHECK(o.getAtPath(du::path(path...))==val);
    }
};
constexpr extraTypeEqT extraTypeEq{};

} // anonymous namespace

template <typename T>
void setSingle()
{
    T o;

    auto check=[&o](const auto& val, const auto& checker, const auto&... path)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::set,val)));
        checker(o,val,path...);
        checkOtherFields(o,path...);
        o.reset();
    };

    check(int8_t(0xaa),normalTypeEq,FieldInt8);
    check(int16_t(0xaabb),normalTypeEq,FieldInt16);
    check(int32_t(0xaabbcc),normalTypeEq,FieldInt32);
    check(int64_t(0xaabbccddee),normalTypeEq,FieldInt64);
    check(uint8_t(0xaa),normalTypeEq,FieldUInt8);
    check(uint16_t(0xaabb),normalTypeEq,FieldUInt16);
    check(uint32_t(0xaabbcc),normalTypeEq,FieldUInt32);
    check(uint64_t(0xaabbccddee),normalTypeEq,FieldUInt64);
    check(true,normalTypeEq,FieldBool);
    check("hello world",normalTypeEq,FieldString);
    check("hi",normalTypeEq,FieldFixedString);
    check(common::DateTime::currentUtc(),extraTypeEq,FieldDateTime);
    check(common::Date::currentUtc(),extraTypeEq,FieldDate);
    check(common::Time::currentUtc(),extraTypeEq,FieldTime);
    check(common::DateRange(common::Date::currentUtc()),extraTypeEq,FieldDateRange);
    check(ObjectId::generateId(),extraTypeEq,FieldObjectId);
    check(plain::MyEnum::Two,extraTypeEq,FieldEnum);
}

template <typename T>
void unsetSingle()
{
    T o;

    auto check=[&o](const auto& val, const auto& checker, const auto&... path)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::set,val)));
        checker(o,val,path...);
        checkOtherFields(o,path...);

        auto member=du::path(path...);
        BOOST_CHECK(o.fieldAtPath(member).isSet());
        checkOtherFields(o,path...);

        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::unset)));
        BOOST_CHECK(!o.fieldAtPath(member).isSet());

        o.reset();
    };

    check(int8_t(0xaa),normalTypeEq,FieldInt8);
    check(int16_t(0xaabb),normalTypeEq,FieldInt16);
    check(int32_t(0xaabbcc),normalTypeEq,FieldInt32);
    check(int64_t(0xaabbccddee),normalTypeEq,FieldInt64);
    check(uint8_t(0xaa),normalTypeEq,FieldUInt8);
    check(uint16_t(0xaabb),normalTypeEq,FieldUInt16);
    check(uint32_t(0xaabbcc),normalTypeEq,FieldUInt32);
    check(uint64_t(0xaabbccddee),normalTypeEq,FieldUInt64);
    check(true,normalTypeEq,FieldBool);
    check("hello world",normalTypeEq,FieldString);
    check("hi",normalTypeEq,FieldFixedString);
    check(common::DateTime::currentUtc(),extraTypeEq,FieldDateTime);
    check(common::Date::currentUtc(),extraTypeEq,FieldDate);
    check(common::Time::currentUtc(),extraTypeEq,FieldTime);
    check(common::DateRange(common::Date::currentUtc()),extraTypeEq,FieldDateRange);
    check(ObjectId::generateId(),extraTypeEq,FieldObjectId);
    check(plain::MyEnum::Two,extraTypeEq,FieldEnum);
}

template <typename T>
void incSingle()
{
    T o;

    auto check=[&o](const auto& val, const auto& checker, const auto&... path)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::set,val)));
        checker(o,val,path...);
        checkOtherFields(o,path...);

        auto member=du::path(path...);
        BOOST_CHECK(o.fieldAtPath(member).isSet());
        checkOtherFields(o,path...);

        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::inc,3)));
        checker(o,val+3,path...);

        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::inc,-3)));
        checker(o,val,path...);
        checkOtherFields(o,path...);

        o.reset();
    };

    check(int8_t(0xaa),normalTypeEq,FieldInt8);
    check(int16_t(0xaabb),normalTypeEq,FieldInt16);
    check(int32_t(0xaabbcc),normalTypeEq,FieldInt32);
    check(int64_t(0xaabbccddee),normalTypeEq,FieldInt64);
    check(uint8_t(0xaa),normalTypeEq,FieldUInt8);
    check(uint16_t(0xaabb),normalTypeEq,FieldUInt16);
    check(uint32_t(0xaabbcc),normalTypeEq,FieldUInt32);
    check(uint64_t(0xaabbccddee),normalTypeEq,FieldUInt64);
}

template <typename T>
void floatingPoint()
{
    T o;

    auto check=[&o](auto val, const auto&... path)
    {
        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::set,val)));
        auto member=du::path(path...);
        BOOST_TEST(o.getAtPath(member)==val, tt::tolerance(0.0001));
        BOOST_CHECK(o.fieldAtPath(member).isSet());
        checkOtherFields(o,path...);

        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::inc,3)));
        BOOST_TEST(o.getAtPath(member)==(val+3), tt::tolerance(0.001));

        update::applyRequest(&o,update::makeRequest(update::Field(update::path(path...),update::inc,-3)));
        BOOST_TEST(o.getAtPath(member)==val, tt::tolerance(0.0001));

        checkOtherFields(o,path...);

        o.reset();
    };

    check(float(100.103),FieldFloat);
    check(double(1000.105),FieldDouble);
}

template <typename T>
void multipleFields()
{
    T o;

    auto check=[&o](const auto& field1, auto val1, const auto& field2, auto val2)
    {
        update::applyRequest(&o,update::request(
                                    update::field(update::path(field1),update::set,val1),
                                    update::field(update::path(field2),update::inc,val2)
                                )
                             );
        BOOST_CHECK_EQUAL(o.getAtPath(field1),val1);
        BOOST_CHECK(o.fieldAtPath(field1).isSet());
        BOOST_CHECK_EQUAL(o.getAtPath(field2),val2);
        BOOST_CHECK(o.fieldAtPath(field2).isSet());
        o.reset();
    };

    check(du::path(FieldInt8),int8_t(0xaa),du::path(FieldInt16),int16_t(0xaabb));
    check(du::path(FieldInt32),int32_t(0xaabbcc),du::path(FieldInt64),int64_t(0xaabbccddee));
    check(du::path(FieldUInt8),uint8_t(0xaa),du::path(FieldUInt16),uint16_t(0xaabb));
    check(du::path(FieldUInt32),uint32_t(0xaabbcc),du::path(FieldUInt64),uint64_t(0xaabbccddee));
}

template <typename T>
void testBytes()
{
    T o;

    std::string val{"Hello world!"};

    std::vector<char> v1;
    std::copy(val.begin(),val.end(),std::back_inserter(v1));
    update::applyRequest(&o,update::request(update::field(update::path(FieldBytes),update::set,v1)));
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).isSet());
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::ByteArray v2;
    v2.append(val);
    update::applyRequest(&o,update::request(update::field(update::path(FieldBytes),update::set,v2)));
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).isSet());
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::VectorOnStack<char> v3;
    v3.append(val);
    update::applyRequest(&o,update::request(update::field(update::path(FieldBytes),update::set,v3)));
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).isSet());
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::pmr::string v4;
    std::copy(val.begin(),val.end(),std::back_inserter(v4));
    update::applyRequest(&o,update::request(update::field(update::path(FieldBytes),update::set,v4)));
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).isSet());
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();

    common::pmr::vector<char> v5;
    std::copy(val.begin(),val.end(),std::back_inserter(v5));
    update::applyRequest(&o,update::request(update::field(update::path(FieldBytes),update::set,v5)));
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).isSet());
    BOOST_CHECK(o.fieldAtPath(du::path(FieldBytes)).equals(lib::string_view{val.data(),val.size()}));
    checkOtherFields(o,FieldBytes);
    o.reset();
}

template <typename T, typename ModelT, typename ClientT>
void checkIndexes(ModelT&& model, std::shared_ptr<ClientT> client)
{
    Topic topic1{"topic1"};

    int16_t val1=100;
    uint32_t val2=1000;

    auto fieldInt16=du::path(FieldInt16);
    auto fieldUInt32=du::path(FieldUInt32);

    auto o1=makeInitObject<T>();
    o1.setAtPath(fieldInt16,val1);
    o1.setAtPath(fieldUInt32,val2);
    BOOST_CHECK_EQUAL(o1.getAtPath(fieldInt16),val1);
    BOOST_CHECK_EQUAL(o1.getAtPath(fieldUInt32),val2);

    // create object in db
    auto ec=client->create(topic1,model,&o1);
    BOOST_REQUIRE(!ec);

    // find object by first field
    auto q1=makeQuery(IdxInt16,query::where(field(fieldInt16),query::eq,val1),topic1);
    auto r1=client->findOne(model,q1);
    BOOST_REQUIRE(!r1);
    BOOST_REQUIRE(!r1.value().isNull());
    BOOST_CHECK_EQUAL(r1.value()->getAtPath(fieldInt16),val1);
    BOOST_CHECK_EQUAL(r1.value()->getAtPath(fieldUInt32),val2);

    // find object by second field
    auto q2=makeQuery(IdxUInt32,query::where(field(fieldUInt32),query::eq,val2),topic1);
    auto r2=client->findOne(model,q2);
    BOOST_REQUIRE(!r2);
    BOOST_REQUIRE(!r2.value().isNull());
    BOOST_CHECK_EQUAL(r2.value()->getAtPath(fieldInt16),val1);
    BOOST_CHECK_EQUAL(r2.value()->getAtPath(fieldUInt32),val2);

    // update object
    int16_t newVal1=200;
    uint32_t incVal2=20;
    auto request=update::request(
        update::field(update::path(fieldInt16),update::set,newVal1),
        update::field(update::path(fieldUInt32),update::inc,incVal2)
        );
    ec=client->updateMany(model,q1,request);
    BOOST_REQUIRE(!ec);

    // find object with prev queries
    auto r3=client->findOne(model,q1);
    BOOST_REQUIRE(!r3);
    BOOST_CHECK(r3.value().isNull());
    r3=client->findOne(model,q2);
    BOOST_REQUIRE(!r3);
    BOOST_CHECK(r3.value().isNull());

    // find object with upated queries
    auto q1_1=makeQuery(IdxInt16,query::where(field(fieldInt16),query::eq,newVal1),topic1);
    auto r1_1=client->findOne(model,q1_1);
    BOOST_REQUIRE(!r1_1);
    BOOST_REQUIRE(!r1_1.value().isNull());
    BOOST_CHECK_EQUAL(r1_1.value()->getAtPath(fieldInt16),newVal1);
    BOOST_CHECK_EQUAL(r1_1.value()->getAtPath(fieldUInt32),val2+incVal2);
    auto q2_1=makeQuery(IdxUInt32,query::where(field(fieldUInt32),query::eq,val2+incVal2),topic1);
    auto r2_1=client->findOne(model,q2_1);
    BOOST_REQUIRE(!r2_1);
    BOOST_REQUIRE(!r2_1.value().isNull());
    BOOST_CHECK_EQUAL(r2_1.value()->getAtPath(fieldInt16),newVal1);
    BOOST_CHECK_EQUAL(r2_1.value()->getAtPath(fieldUInt32),val2+incVal2);
}
