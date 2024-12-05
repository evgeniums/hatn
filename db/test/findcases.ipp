/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/findcases.ipp
*/

/****************************************************************************/

BOOST_AUTO_TEST_CASE(Eq)
{
#ifndef HATN_DISABLE_MANUAL_QUERY_TESTS
    std::ignore=makeQuery(IdxString,query::where(field(FieldString),query::eq,"hi"),topic());
    std::ignore=makeQuery(IdxString,query::where(field(FieldString),query::eq,lib::string_view("hi")),topic());
    // std::ignore=makeQuery(IdxString,query::where(FieldString,query::eq,std::string("hi")),topic());
    std::string val("hi");
    std::ignore=makeQuery(IdxString,query::where(field(FieldString),query::eq,val),topic());
    std::ignore=makeQuery(IdxFixedString,query::where(field(FieldFixedString),query::eq,plain::MyEnum::One),topic());
#endif
    InvokeTestT<eqQueryGenT<>,eqCheckerT> testEq{eqQueryGen<>,eqChecker};
    runTest(testEq);
}

BOOST_AUTO_TEST_CASE(Neq)
{
    InvokeTestT<eqQueryGenT<true>,neqCheckerT> testNeq{eqQueryGen<true>,neqChecker};
    runTest(testNeq);
}

BOOST_AUTO_TEST_CASE(Lt)
{
    InvokeTestT<ltQueryGen,ltChecker> testLt{ltQueryGen(),ltChecker()};
    runTest(testLt);
}

BOOST_AUTO_TEST_CASE(Lte)
{
    InvokeTestT<ltQueryGen,ltChecker> testLte{ltQueryGen(true),ltChecker(true)};
    runTest(testLte);
}

BOOST_AUTO_TEST_CASE(Gt)
{
    InvokeTestT<gtQueryGen,gtChecker> testGt{gtQueryGen(),gtChecker()};
    runTest(testGt);
}

BOOST_AUTO_TEST_CASE(Gte)
{
    InvokeTestT<gtQueryGen,gtChecker> testGte{gtQueryGen(true),gtChecker(true)};
    runTest(testGte);
}

BOOST_AUTO_TEST_CASE(InVector)
{
    // std::ignore=query::where(object::_id,query::in,{1,2,3,4,5});
    std::vector<int> v{1,2,3,4,5};
    std::ignore=query::where(object::_id,query::in,v);

    InvokeTestT<inVectorQueryGenT<>,inVectorCheckerT> testInVector{inVectorQueryGen<>,inVectorChecker};
    runTest(testInVector,hana::true_c);
}

BOOST_AUTO_TEST_CASE(NinVector)
{
    InvokeTestT<inVectorQueryGenT<true>,ninVectorCheckerT> testNinVector{inVectorQueryGen<true>,ninVectorChecker};
    runTest(testNinVector,hana::true_c);
}

template <bool Nin>
void inNinInterval()
{
    query::IntervalType fromType{query::IntervalType::Open};
    query::IntervalType toType{query::IntervalType::Open};

    std::ignore=query::Interval<query::String>{"1000",fromType,"9000",toType};
    // std::ignore=query::Interval<query::String>{std::string("1000"),fromType,"9000",toType};
    // std::ignore=query::Interval<query::String>{"1000",fromType,std::string("9000"),toType};
    std::string from("1000");
    std::string to("9000");
    std::ignore=query::Interval<query::String>{from,fromType,to,toType};

    auto run=[&]()
    {
        BOOST_TEST_MESSAGE(fmt::format("[{},{}]",query::intervalTypeToString(fromType),query::intervalTypeToString(toType)));

        InvokeTestT<inIntervalQueryGenT<Nin>,inIntervalCheckerT<Nin>> test{
            inIntervalQueryGenT<Nin>{
                fromType,
                toType
            },
            inIntervalCheckerT<Nin>{
                fromType,
                toType
            }
        };
        runTest(test,hana::true_c);
    };

    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Open};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Open};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Open};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Closed};
    run();

    fromType=query::IntervalType{query::IntervalType::Open};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    fromType=query::IntervalType{query::IntervalType::Closed};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    fromType=query::IntervalType{query::IntervalType::First};
    toType=query::IntervalType{query::IntervalType::Last};
    run();

    if (!Nin)
    {
        fromType=query::IntervalType{query::IntervalType::First};
        toType=query::IntervalType{query::IntervalType::First};
        run();

        fromType=query::IntervalType{query::IntervalType::Last};
        toType=query::IntervalType{query::IntervalType::Last};
        run();
    }
}

BOOST_AUTO_TEST_CASE(InInterval)
{
    inNinInterval<false>();
}

BOOST_AUTO_TEST_CASE(NinInterval)
{
    inNinInterval<true>();
}

BOOST_AUTO_TEST_CASE(FindFirst)
{
    InvokeTestT<eqFirstLastQueryGenT<>,eqFirstLastCheckerT<>> testFirst{eqFirstLastQueryGen<>,eqFirstLastChecker<>};
    runTest(testFirst,hana::true_c);
}

BOOST_AUTO_TEST_CASE(FindLast)
{
    InvokeTestT<eqFirstLastQueryGenT<true>,eqFirstLastCheckerT<true>> testLast{eqFirstLastQueryGen<true>,eqFirstLastChecker<true>};
    runTest(testLast,hana::true_c);
}
